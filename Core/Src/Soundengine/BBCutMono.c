/*
 * Livecut-inspired mono master phrase engine for LXR Eurorack.
 * The CutProc11 scheduling and weighted end-of-phrase fill rules are
 * adapted from Remy Muller's Livecut (GPL-2.0-or-later).
 * Original: https://github.com/scheffle/Livecut/blob/master/lib/BBCutter.cpp
 *
 * This is a FIRST PORT of CutProc11, not a complete Livecut port.
 * Livecut's WarpCut/SQPusher, amplitude/pitch modulation, and original
 * LivePlayer envelope/transport implementation are not yet integrated.
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
#include "Soundengine/BBCutMono.h"
#include <string.h>

/* Two separate buffers: a recent audio history and a frozen fragment.
 * 32768 / 44140 = 0.742 seconds, sufficient for 3/8 at 140 BPM.
 * A separate frozen cut prevents later repeats from following the live
 * ring write head or being overwritten during longer phrase repetitions.
 * RAM cost: 128 KiB, up from the old engine's 32 KiB.
 */
#define LC_CAPACITY 32768u
#define LC_MASK (LC_CAPACITY - 1u)
#define LC_FS 44140u
#define LC_SUBDIV 8u /* units per 4/4 bar; one unit is an eighth note */
#define LC_MAX_CUTS 512u

static int16_t history[LC_CAPACITY];
static int16_t frozen[LC_CAPACITY];
static uint32_t write_pos, history_valid;
static uint32_t frozen_len, cut_pos, unit_samples;
static uint32_t repeats_remaining, phrase_done, phrase_total;
static uint32_t phrase_bars, selected_units, capture_pending;
static uint32_t seed = 0x12a74b19u;
static uint32_t last_rise_ms, pulse_ms;
static uint8_t ppqn = 1u, gate, valid_clock;
static float tempo = 120.0f;
static uint8_t enabled_previous;

/* xorshift32: fixed memory, deterministic, safe in the audio callback. */
static uint32_t lc_rand(void) {
    seed ^= seed << 13; seed ^= seed >> 17; seed ^= seed << 5;
    return seed;
}
static float lc_random(void) {
    return (float)(lc_rand() & 0xFFFFFFu) / 16777216.0f;
}
/* Match Livecut Math::randominteger's inclusive, rounded selection. */
static uint32_t lc_random_integer(uint32_t a, uint32_t b) {
    if (a >= b) return a;
    return a + (uint32_t)(0.5000001f + (float)(b-a)*lc_random());
}
static void lc_begin_phrase(void) {
    /* Livecut CutProc default 1-4 bar phrases. */
    phrase_bars = lc_random_integer(1u,4u);
    phrase_total = phrase_bars * LC_SUBDIV;
    phrase_done = 0u;
}
static uint32_t lc_samples_per_unit(void) {
    float bpm = tempo;
    if (bpm < 20.0f) bpm=20.0f;
    if (bpm > 300.0f) bpm=300.0f;
    float value = ((float)LC_FS * 60.0f / bpm) * (4.0f / LC_SUBDIV);
    if(value < 64.0f) value=64.0f;
    return (uint32_t)(value+0.5f);
}
static void lc_choose_block(void) {
    if (phrase_done >= phrase_total || phrase_total==0u) lc_begin_phrase();
    uint32_t left = phrase_total - phrase_done;
    uint32_t units = 1u, repeats = 1u;
    uint32_t samples = lc_samples_per_unit();
    float fractional_units = 1.0f;
    /* Real Livecut CutProc11: last-half-bar stutter area and
     * weighted repeat multipliers {1,2,3,4,6,8}. Defaults:
     * stutterchance=1; stutterarea=0.5; repeats=1..2.
     */
    if ((float)left/(float)LC_SUBDIV < 0.5f && lc_random() < 1.0f) {
        const uint8_t choices[] = {1,2,3,4,6,8};
        const float weights[] = {0.40f,0.30f,0.10f,0.10f,0.05f,0.05f};
        float pick=lc_random(), sum=0.0f;
        uint32_t multiplier=1u;
        for(uint32_t i=0;i<6;i++) {
            sum += weights[i];
            if(pick < sum) { multiplier = choices[i]; break; }
        }
        repeats = left * multiplier;
        fractional_units = 1.0f/(float)multiplier;
        units = left;
    } else {
        /* Livecut's odd-unit cut chooser:
         * 2 * long(random(0, subdiv/4) + 0.5) + 1
         */
        units = 2u*(uint32_t)(2.0f*lc_random()+0.5f)+1u;
        repeats = lc_random_integer(1u,2u);
        while(units > left && units > 1u) units-=2u;
        while(repeats * units > left) {
            if (repeats>1u) repeats--;
            else if (left <= (LC_SUBDIV/2u+1u)) {units=left; break;}
            else {units=1u; break;}
        }
        fractional_units=(float)units;
    }
    if (repeats > LC_MAX_CUTS) repeats=LC_MAX_CUTS;
    uint32_t length=(uint32_t)(fractional_units * (float)samples);
    if(length < 32u) length=32u;
    if(length > LC_CAPACITY) length=LC_CAPACITY;
    frozen_len=length;
    selected_units=units;
    repeats_remaining=repeats;
    /* The phrase progresses by total played musical units. */
    phrase_done += units;
    capture_pending=1u;
    cut_pos=0u;
}
static void lc_capture(void) {
    /* Capturing an already-heard fragment rather than future audio avoids
     * waiting a whole cut before the first stutter can start. The source
     * fragment stays fixed across every repeat in this block. */
    if(history_valid < frozen_len) {
        capture_pending=0u;
        repeats_remaining=0u;
        return;
    }
    uint32_t start=(write_pos + LC_CAPACITY - frozen_len) & LC_MASK;
    for(uint32_t i=0;i<frozen_len;i++) frozen[i]=history[(start+i)&LC_MASK];
    cut_pos=0u;
    capture_pending=0u;
}
void BBCut_init(void) {
    memset(history,0,sizeof(history));
    memset(frozen,0,sizeof(frozen));
    write_pos=history_valid=frozen_len=cut_pos=0u;
    unit_samples=repeats_remaining=phrase_done=phrase_total=0u;
    phrase_bars=selected_units=capture_pending=0u;
    last_rise_ms=pulse_ms=0u;
    ppqn=1u; gate=valid_clock=enabled_previous=0u;
    tempo=120.0f;
}
void BBCut_setPpqn(uint8_t n) {
    if(n==1u || n==2u || n==4u || n==8u || n==16u || n==24u) {
        if(ppqn!=n) {ppqn=n;valid_clock=0u;last_rise_ms=0u;}
    }
}
void BBCut_clockSample(float cv,uint32_t now) {
    if(!gate && cv>0.60f) {
        gate=1u;
        if(last_rise_ms) {
            uint32_t dt=now-last_rise_ms;
            if(dt>=3u && dt<4000u) {
                float next=60000.0f/((float)dt*(float)ppqn);
                if(next>=20.0f && next<=300.0f) {
                    pulse_ms=dt;
                    tempo=valid_clock?tempo*0.75f+next*0.25f:next;
                    valid_clock=1u;
                }
            }
        }
        last_rise_ms=now;
    } else if(gate && cv<0.35f) gate=0u;
}
uint8_t BBCut_clockActive(uint32_t now) {
    if(!valid_clock || !pulse_ms) {enabled_previous=0u;return 0u;}
    uint32_t timeout=pulse_ms*3u;
    if(timeout<500u)timeout=500u;
    if(timeout>5000u)timeout=5000u;
    if(now-last_rise_ms>timeout) {
        valid_clock=0u;
        enabled_previous=0u;
        return 0u;
    }
    return 1u;
}
float BBCut_bpm(void) {return tempo;}
void BBCut_processStereo(int16_t *out,uint32_t frames) {
    if(!enabled_previous) {
        repeats_remaining=0u;
        phrase_total=0u;
        enabled_previous=1u;
    }
    for(uint32_t i=0u;i<frames;i++) {
        const int32_t left=out[2u*i], right=out[2u*i+1u];
        const int16_t mono=(int16_t)((left+right)/2);
        if(repeats_remaining==0u)lc_choose_block();
        if(capture_pending)lc_capture();
        int16_t value=mono;
        if(repeats_remaining && frozen_len) {
            value=frozen[cut_pos];
            cut_pos++;
            if(cut_pos>=frozen_len) {
                cut_pos=0u;
                repeats_remaining--;
            }
        }
        /* Smooth edges at cut boundaries, without spending CPU on
         * pitch-shifting / time-stretching. Short 32-frame fades avoid clicks.
         * Fading occurs against the real input signal at the edges. */
        if(repeats_remaining && frozen_len>64u) {
            uint32_t edge=cut_pos < (frozen_len-cut_pos) ? cut_pos : (frozen_len-cut_pos);
            if(edge<32u) {
                uint32_t dry=32u-edge;
                value=(int16_t)(((int32_t)value*(int32_t)edge+(int32_t)mono*(int32_t)dry)/32);
            }
        }
        out[2u*i]=value;
        out[2u*i+1u]=value;
        history[write_pos]=mono;
        write_pos=(write_pos+1u)&LC_MASK;
        if(history_valid<LC_CAPACITY)history_valid++;
    }
}
