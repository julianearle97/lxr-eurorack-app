/* EXPERIMENTAL ONLY: CV1 ADC scan -> Output 3, gated by Drum 1.
 * CV input NOT designed for audio. Expect severe aliasing/rough lo-fi sound.
 * Does not change ADC configuration, keeps normal CV voltage protections.
 */
#include "Soundengine/CvSidechainExperiment.h"
#include "IO/adcPots.h"
#include "Soundengine/CvAudioCapture.h"
#include <stdint.h>
static volatile uint8_t kick_pending;
static float duck=1.0f;
static float dc=0.0f;
static float last=0.0f;
static volatile uint8_t duck_amount=127, highpass=0, input_gain=127, release=60;
static float hp_low=0.0f;
void cv_sidechain_set(uint8_t d,uint8_t hp,uint8_t g,uint8_t r){duck_amount=d;highpass=hp;input_gain=g;release=r;}
void cv_sidechain_kick(void){kick_pending=1;}
void cv_sidechain_mix(int16_t *output3_interleaved,uint32_t frames){
    if(!output3_interleaved)return;
    if(kick_pending){kick_pending=0; duck=1.0f-((float)duck_amount/127.0f);}
    /* The normal ADC DMA scans 13 pots/CVs, not an audio-rate stream.
       Capture its most recent CV1 value once per block as a deliberately
       degraded proof-of-concept. DC blocking avoids enormous offset. */
    /* Stream actual ADC CV1 conversions rather than one per audio block. */
    /* 12..350ms one-pole recovery. Log-spaced high pass from 20..1600Hz. */
    const float release_ms=12.0f+338.0f*((float)release/127.0f)*((float)release/127.0f);
    const float recover=1.0f/(44.1f*release_ms);
    const float cutoff=20.0f+(float)highpass*(float)highpass*0.098f;
    const float hp_coef=(6.2831853f*cutoff)/(44100.0f+6.2831853f*cutoff);
    const float makeup=(float)input_gain/127.0f;
    for(uint32_t i=0;i<frames;i++){
       float x=(2048.0f-cv_audio_capture_read())*8.0f;
       dc+=0.001f*(x-dc);
       float s=x-dc;
       duck+=(1.0f-duck)*recover;
       hp_low+=hp_coef*(s-hp_low);
       s=(s-hp_low)*makeup;
       int32_t y=(int32_t)(s*duck);
       int32_t current=output3_interleaved[2*i];
       y+=current;
       if(y>32767)y=32767;
       if(y< -32768)y=-32768;
       output3_interleaved[2*i]=(int16_t)y;
    }
    last=0.0f;
}
