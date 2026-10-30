/*
 * Experimental mono rumble processor for Drum 1.
 * Copyright (C) 2026. SPDX-License-Identifier: GPL-3.0-only
 * Prototype only: not yet connected to the LXR mixer or UI.
 */
#ifndef LXR_KICK_RUMBLE_H
#define LXR_KICK_RUMBLE_H

#include <stdint.h>

/* Approx. 45 KB; static allocation only, no heap. */
#define KICK_RUMBLE_D1 2903
#define KICK_RUMBLE_D2 3371
#define KICK_RUMBLE_D3 4157
#define KICK_RUMBLE_AP 421

typedef struct {
    float delay1[KICK_RUMBLE_D1];
    float delay2[KICK_RUMBLE_D2];
    float delay3[KICK_RUMBLE_D3];
    float diffuser[KICK_RUMBLE_AP];
    uint32_t p1, p2, p3, pa;
    float hp_state, lp_state, duck_gain;
    float hp_coef, lp_coef, feedback, level, duck_amount;
    float duck_release_coef;
} KickRumble;

/* All values normalized [0, 1] except filter cutoff frequencies in Hz. */
void KickRumble_init(KickRumble *r, float sample_rate);
void KickRumble_set(KickRumble *r, float sample_rate, float level,
                    float decay, float hp_hz, float lp_hz, float duck);
void KickRumble_trigger(KickRumble *r);
/* Process one sample of mono Drum 1 output, return dry + wet mono. */
float KickRumble_process(KickRumble *r, float dry);
#endif
