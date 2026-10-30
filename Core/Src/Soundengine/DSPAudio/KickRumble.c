/*
 * Experimental mono kick rumble DSP.
 * Copyright (C) 2026. SPDX-License-Identifier: GPL-3.0-only
 * Prototype, not yet wired into audio routing or parameter storage.
 */
#include "KickRumble.h"
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
    r->feedback = 0.68f + 0.285f * clamp(decay, 0.0f, 1.0f);
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
    float input = 0.28f * dry;
    /* Three mutually-fed delays, with bounded feedback and one diffuser. */
    r->delay1[r->p1] = clamp(input + r->feedback * (0.55f*b + 0.45f*c), -1.0f, 1.0f);
    r->delay2[r->p2] = clamp(input + r->feedback * (0.55f*c - 0.45f*a), -1.0f, 1.0f);
    r->delay3[r->p3] = clamp(input + r->feedback * (0.55f*a + 0.45f*b), -1.0f, 1.0f);
    if (++r->p1 == KICK_RUMBLE_D1) r->p1 = 0;
    if (++r->p2 == KICK_RUMBLE_D2) r->p2 = 0;
    if (++r->p3 == KICK_RUMBLE_D3) r->p3 = 0;
    float wet = 0.45f*a + 0.35f*b + 0.20f*c;
    float ap = r->diffuser[r->pa];
    float diffused = ap - 0.5f*wet;
    r->diffuser[r->pa] = wet + 0.5f*diffused;
    if (++r->pa == KICK_RUMBLE_AP) r->pa = 0;
    /* First-order high pass and low pass; coefficients set outside audio thread. */
    r->hp_state += r->hp_coef * (diffused - r->hp_state);
    float hp = diffused - r->hp_state;
    r->lp_state += r->lp_coef * (hp - r->lp_state);
    r->duck_gain += r->duck_release_coef * (1.0f - r->duck_gain);
    return clamp(dry + r->level * r->duck_gain * r->lp_state, -1.0f, 1.0f);
}
