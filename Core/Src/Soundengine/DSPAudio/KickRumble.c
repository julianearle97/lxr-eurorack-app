/*
 * Experimental mono kick rumble DSP.
 * Copyright (C) 2026. SPDX-License-Identifier: GPL-3.0-only
 * Experimental V2: denser long-decay mono rumble network.
 */
#include "Soundengine/DSPAudio/KickRumble.h"
#include <math.h>
#include <string.h>

static float clamp(float x, float low, float high) {
    if (x < low) return low;
    if (x > high) return high;
    return x;
}
static float pole(float hz, float sr) {
    return 1.0f - expf(-6.28318530718f * hz / sr);
}
void KickRumble_init(KickRumble *r, float sr) {
    memset(r, 0, sizeof(*r));
    r->duck_gain = 1.0f;
    KickRumble_set(r, sr, 0.0f, 0.5f, 35.0f, 160.0f, 0.65f);
}
void KickRumble_set(KickRumble *r, float sr, float level,
                    float decay, float hp_hz, float lp_hz, float duck) {
    if (sr < 8000.0f) sr = 44100.0f;
    r->level = clamp(level, 0.0f, 1.0f);
    /* Near-unity orthogonal feedback gives a much longer, denser low-end tail. */
    /* Map knob exponentially to approximately 0.12 .. 6.0 seconds RT60.
     * For a ~80 ms delay circuit, -60 dB after rt60 is g^N=0.001.
     * This is a much broader response than V2's 0.78 .. 0.988. */
    const float d = clamp(decay, 0.0f, 1.0f);
    const float rt60 = 0.12f * powf(50.0f, d);
    const float mean_delay_s = ((2903.0f + 3371.0f + 4157.0f) / 3.0f) / sr;
    r->feedback = expf(-6.9077553f * mean_delay_s / rt60);
    hp_hz = clamp(hp_hz, 15.0f, sr * 0.2f);
    lp_hz = clamp(lp_hz, hp_hz + 5.0f, sr * 0.45f);
    r->hp_coef = pole(hp_hz, sr);
    r->lp_coef = pole(lp_hz, sr);
    r->bass_coef = pole(100.0f, sr);
    r->duck_amount = clamp(duck, 0.0f, 1.0f);
    /* 2 ms attack, ~150 ms exponential recovery, and 90 ms full-depth hold.
     * Keep the wet gain near zero at maximum duck, not merely a subtle dip. */
    r->duck_attack_coef = pole(500.0f, sr);
    r->duck_release_coef = pole(6.5f, sr);
    r->duck_hold_length = (uint32_t)(sr * 0.090f);
    if (r->duck_amount == 0.0f) {
        r->duck_hold_left = 0;
        r->duck_target = 1.0f;
        r->duck_gain = 1.0f;
    }
}
void KickRumble_trigger(KickRumble *r) {
    if (r->duck_amount > 0.0f) {
        /* Nonlinear perceived depth: 50% knob means ~75% attenuation. */
        const float remaining = 1.0f - r->duck_amount;
        r->duck_target = remaining * remaining;
        r->duck_hold_left = r->duck_hold_length;
    }
}
float KickRumble_process(KickRumble *r, float dry) {
    /* Always advance when enabled; disable bypass leaves dry unchanged. */
    if (r->level <= 0.0f) return dry;
    float a = r->delay1[r->p1];
    float b = r->delay2[r->p2];
    float c = r->delay3[r->p3];
    /* Householder feedback matrix: energy-preserving before feedback gain.
     * This prevents the V1 0.55/0.45 weighted feedback from dying too quickly.
     * Inject a stronger kick excitation, kept within saturation bounds. */
    const float sum = (a + b + c) * (2.0f / 3.0f);
    const float input = 0.65f * dry;
    const float f1 = r->feedback * (a - sum);
    const float f2 = r->feedback * (b - sum);
    const float f3 = r->feedback * (c - sum);
    r->delay1[r->p1] = clamp(input + f1, -1.0f, 1.0f);
    r->delay2[r->p2] = clamp(input + f2, -1.0f, 1.0f);
    r->delay3[r->p3] = clamp(input + f3, -1.0f, 1.0f);
    if (++r->p1 == KICK_RUMBLE_D1) r->p1 = 0;
    if (++r->p2 == KICK_RUMBLE_D2) r->p2 = 0;
    if (++r->p3 == KICK_RUMBLE_D3) r->p3 = 0;
    /* Positive output taps help preserve low frequencies instead of cancelling. */
    float wet = (a + b + c) * (1.0f / 3.0f);
    float ap = r->diffuser[r->pa];
    float diffused = ap - 0.6f*wet;
    r->diffuser[r->pa] = wet + 0.6f*diffused;
    if (++r->pa == KICK_RUMBLE_AP) r->pa = 0;
    /* First-order high pass and low pass; coefficients set outside audio thread. */
    r->hp_state += r->hp_coef * (diffused - r->hp_state);
    float hp = diffused - r->hp_state;
    r->lp_state += r->lp_coef * (hp - r->lp_state);
    r->lp_state2 += r->lp_coef * (r->lp_state - r->lp_state2);
    /* Low bass reinforcement: filtered rumble at 100 Hz, never the dry kick. */
    r->bass_state += r->bass_coef * (r->lp_state2 - r->bass_state);
    if (r->duck_hold_left > 0) {
        --r->duck_hold_left;
        r->duck_gain += r->duck_attack_coef * (r->duck_target - r->duck_gain);
    } else {
        r->duck_gain += r->duck_release_coef * (1.0f - r->duck_gain);
    }
    /* 40 ms wet-only pre-delay creates a distinct low-end bounce after the kick. */
    const float delayedWet = r->predelay[r->pp];
    r->predelay[r->pp] = 1.25f * r->lp_state2 + 1.0f * r->bass_state;
    if (++r->pp == KICK_RUMBLE_PREDELAY) r->pp = 0;
    /* Wet-only drive adds weight/harmonics; bounded to avoid runaway output. */
    const float thick = clamp(delayedWet * 2.8f, -2.0f, 2.0f);
    const float saturated = thick / (1.0f + 0.35f * fabsf(thick));
    return clamp(dry + r->level * r->duck_gain * saturated, -1.0f, 1.0f);
}
