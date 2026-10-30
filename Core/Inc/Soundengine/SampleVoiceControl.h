#ifndef LXR_SAMPLE_VOICE_CONTROL_H
#define LXR_SAMPLE_VOICE_CONTROL_H
#include <stdint.h>
void SampleVoiceControl_init(void);
void SampleVoiceControl_set(uint8_t voice,uint8_t enabled,uint8_t sample,uint8_t pitch,uint8_t volume);
uint8_t SampleVoiceControl_isSample(uint8_t voice);
void SampleVoiceControl_trigger(uint8_t voice,uint8_t velocity);
void SampleVoiceControl_render(uint8_t voice,int16_t *out,uint32_t frames);
uint8_t SampleVoiceControl_activeHat(void);
#endif
