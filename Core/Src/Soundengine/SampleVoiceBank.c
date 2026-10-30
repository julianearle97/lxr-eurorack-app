#include "Soundengine/SampleVoiceBank.h"
#include <math.h>
#include <string.h>
void SampleVoiceBank_init(SampleVoiceBank *b) {
    memset(b,0,sizeof(*b));
    for(unsigned i=0;i<SAMPLE_VOICE_COUNT;i++)
        b->voices[i].phase_q16=UINT64_MAX;
}
void SampleVoiceBank_assign(SampleVoiceBank *b,uint8_t id,const int8_t *data,uint32_t length) {
    if(id>=SAMPLE_SLOT_COUNT)return;
    b->slots[id]=(SampleSlot){data,length};
}
void SampleVoiceBank_setVoice(SampleVoiceBank *b,uint8_t voice,uint8_t enabled,
                              uint8_t sample_id,uint8_t pitch,uint8_t volume) {
    if(voice>=SAMPLE_VOICE_COUNT)return;
    SampleVoiceState *v=&b->voices[voice];
    v->enabled=!!enabled;
    v->sample_id=sample_id%SAMPLE_SLOT_COUNT;
    v->volume=volume>127?127:volume;
    /* Pitch 64 = original speed; 0/127 ~= -24/+24 semitones. */
    int32_t semitones=((int32_t)pitch-64)*24/63;
    float ratio=exp2f((float)semitones/12.0f);
    v->increment_q16=(uint32_t)(65536.0f*ratio);
    v->phase_q16=UINT64_MAX;
}
void SampleVoiceBank_trigger(SampleVoiceBank *b,uint8_t voice,uint8_t velocity) {
    if(voice>=SAMPLE_VOICE_COUNT)return;
    SampleVoiceState *v=&b->voices[voice];
    if(!v->enabled)return;
    v->velocity=velocity>127?127:velocity;
    v->phase_q16=0;
}
void SampleVoiceBank_stop(SampleVoiceBank *b,uint8_t voice) {
    if(voice<SAMPLE_VOICE_COUNT)b->voices[voice].phase_q16=UINT64_MAX;
}
void SampleVoiceBank_render(SampleVoiceBank *b,uint8_t voice,int16_t *dst,uint32_t count) {
    if(!dst)return;
    memset(dst,0,count*sizeof(*dst));
    if(voice>=SAMPLE_VOICE_COUNT)return;
    SampleVoiceState *v=&b->voices[voice];
    if(!v->enabled)return;
    const SampleSlot *slot=&b->slots[v->sample_id];
    if(!slot->data || !slot->length)return;
    for(uint32_t i=0;i<count;i++) {
        uint64_t idx=v->phase_q16>>16;
        if(idx>=slot->length)break;
        uint32_t frac=(uint32_t)(v->phase_q16&65535);
        int32_t a=slot->data[idx], b2=slot->data[idx+1<slot->length?idx+1:idx];
        int32_t interpolated=a+((b2-a)*(int32_t)frac>>16);
        /* Signed PCM8 -> 16-bit output, volume and input velocity. */
        int32_t value=(interpolated*256*(int32_t)v->volume*(int32_t)v->velocity)/(127*127);
        if(value>32767)value=32767;
        if(value<(-32768))value=-32768;
        dst[i]=(int16_t)value;
        v->phase_q16+=v->increment_q16;
    }
}
