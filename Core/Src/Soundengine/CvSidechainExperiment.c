/* EXPERIMENTAL ONLY: CV1 ADC scan -> Output 3, gated by Drum 1.
 * CV input NOT designed for audio. Expect severe aliasing/rough lo-fi sound.
 * Does not change ADC configuration, keeps normal CV voltage protections.
 */
#include "Soundengine/CvSidechainExperiment.h"
#include "IO/adcPots.h"
#include <stdint.h>
static volatile uint8_t kick_pending;
static float duck=1.0f;
static float dc=0.0f;
static float last=0.0f;
void cv_sidechain_kick(void){kick_pending=1;}
void cv_sidechain_mix(int16_t *output3_interleaved,uint32_t frames){
    if(!output3_interleaved)return;
    if(kick_pending){kick_pending=0; duck=0.0f;}
    /* The normal ADC DMA scans 13 pots/CVs, not an audio-rate stream.
       Capture its most recent CV1 value once per block as a deliberately
       degraded proof-of-concept. DC blocking avoids enormous offset. */
    float v=(float)(adc_buf[8]>>4);
    float x=(2048.0f-v)*8.0f;
    dc+=0.004f*(x-dc);
    x-=dc;
    /* Gain recovers in about 180ms at the LXR's 44.1kHz output rate. */
    for(uint32_t i=0;i<frames;i++){
       float blend=(float)i/(float)(frames?frames:1);
       float s=last+(x-last)*blend;
       duck+=(1.0f-duck)*0.000126f;
       int32_t y=(int32_t)(s*duck);
       int32_t current=output3_interleaved[2*i];
       y+=current;
       if(y>32767)y=32767;
       if(y< -32768)y=-32768;
       output3_interleaved[2*i]=(int16_t)y;
    }
    last=x;
}
