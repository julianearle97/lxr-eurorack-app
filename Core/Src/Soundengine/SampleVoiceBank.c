#include "Soundengine/SampleVoiceBank.h"
#include "Soundengine/DSPAudio/transientTables.h"
#include <string.h>
void SampleVoiceBank_init(SampleVoiceBank *b) {memset(b,0,sizeof(*b));}
void SampleVoiceBank_setVoice(SampleVoiceBank *b,uint8_t voice,uint8_t enabled,uint8_t sample_id) {
    if(voice>=SAMPLE_VOICE_COUNT) return;
    b->voices[voice].enabled=enabled?1:0;
    b->voices[voice].sample_id=(uint8_t)(sample_id%NUM_TRANSIENTS);
    b->voices[voice].phase=TRANSIENT_SAMPLE_LENGTH;
}
void SampleVoiceBank_trigger(SampleVoiceBank *b,uint8_t voice,uint8_t velocity) {
    if(voice>=SAMPLE_VOICE_COUNT || !b->voices[voice].enabled)return;
    b->voices[voice].phase=0;
    b->voices[voice].gain=velocity;
}
void SampleVoiceBank_render(SampleVoiceBank *b,uint8_t voice,int16_t *dst,uint32_t count) {
    if(!dst || voice>=SAMPLE_VOICE_COUNT)return;
    SampleVoiceState *v=&b->voices[voice];
    memset(dst,0,count*sizeof(*dst));
    if(!v->enabled)return;
    for(uint32_t i=0;i<count;i++) {
        if(v->phase>=TRANSIENT_SAMPLE_LENGTH)break;
        /* Original factory transients are 8-bit signed, ~50 ms @44.1k.
         * Read-only bank; one-shot playback has no oscillator envelope. */
        int32_t s=transientData[v->sample_id][v->phase++];
        dst[i]=(int16_t)(s*(int32_t)v->gain*2);
    }
}
