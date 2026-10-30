/* Unofficial custom firmware: sample render dispatcher.
 * PCM8 data is const flash memory; missing bank placeholder plays silence.
 */
#include "Soundengine/SampleVoiceControl.h"
#include "Soundengine/SampleVoiceBank.h"
#include "Soundengine/UserSampleBank.h"
#include <stdint.h>
static SampleVoiceBank bank;
static uint8_t initialized=0;
static uint8_t active_hat=5;
void SampleVoiceControl_init(void){
    if(initialized)return;
    SampleVoiceBank_init(&bank);
    for(unsigned i=0;i<12;i++)
        if(lxr_custom_sample_lengths[i])
            SampleVoiceBank_assign(&bank,i,
              lxr_custom_samples+lxr_custom_sample_offsets[i],
              lxr_custom_sample_lengths[i]);
    initialized=1;
}
void SampleVoiceControl_set(uint8_t voice,uint8_t enabled,uint8_t slot,uint8_t pitch,uint8_t volume){
    if(voice>=7)return;
    SampleVoiceControl_init();
    SampleVoiceBank_setVoice(&bank,voice,enabled,slot,pitch,volume);
}
uint8_t SampleVoiceControl_isSample(uint8_t voice){
    if(voice>=7)return 0;
    SampleVoiceControl_init();
    return bank.voices[voice].enabled;
}
void SampleVoiceControl_trigger(uint8_t voice,uint8_t velocity){
    if(voice>=7)return;
    SampleVoiceControl_init();
    if(voice==5||voice==6){
        active_hat=voice;
        if(voice==5)SampleVoiceBank_stop(&bank,6); /* closed hat chokes open sample */
        else SampleVoiceBank_stop(&bank,5);
    }
    SampleVoiceBank_trigger(&bank,voice,velocity);
}
void SampleVoiceControl_render(uint8_t voice,int16_t *out,uint32_t frames){
    SampleVoiceControl_init();
    SampleVoiceBank_render(&bank,voice,out,frames);
}
uint8_t SampleVoiceControl_activeHat(void){return active_hat;}
