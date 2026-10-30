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
    r->feedback = 0.78f + 0.208f * clamp(decay, 0.0f, 1.0f);
    hp_hz = clamp(hp_hz, 15.0f, sr * 0.2f);
    lp_hz = clamp(lp_hz, hp_hz + 5.0f, sr * 0.45f);
    r->hp_coef = pole(hp_hz, sr);
    r->lp_coef = pole(lp_hz, sr);
    r->duck_amount = clamp(duck, 0.0f, 1.0f);
    /* 100 ms release; eventually map this to kick timing. */
    r->duck_release_coef = pole(10.0f, sr);
}
void KickRumble_trigger(KickRumble *r) {
    r->duck_gain = 1.0f - r->duck_amount;
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
    r->duck_gain += r->duck_release_coef * (1.0f - r->duck_gain);
    /* Wet-only drive adds weight/harmonics; bounded to avoid runaway output. */
    const float thick = clamp(r->lp_state * 2.8f, -2.0f, 2.0f);
    const float saturated = thick / (1.0f + 0.35f * fabsf(thick));
    return clamp(dry + r->level * r->duck_gain * saturated, -1.0f, 1.0f);
}
