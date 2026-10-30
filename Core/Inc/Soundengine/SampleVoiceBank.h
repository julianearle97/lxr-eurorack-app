/* Custom LXR prototype: selectable one-shot bank. GPL-3.0-only.
 * No hardware integration yet; existing synth voices remain unchanged.
 */
#ifndef LXR_SAMPLE_VOICE_H
#define LXR_SAMPLE_VOICE_H
#include <stdint.h>
#define SAMPLE_VOICE_COUNT 7
typedef struct {
    uint32_t phase;
    uint8_t sample_id;
    uint8_t enabled;
    uint8_t gain;
} SampleVoiceState;
typedef struct {
    SampleVoiceState voices[SAMPLE_VOICE_COUNT];
} SampleVoiceBank;
void SampleVoiceBank_init(SampleVoiceBank *bank);
void SampleVoiceBank_setVoice(SampleVoiceBank *bank,uint8_t voice,uint8_t enabled,uint8_t sample_id);
void SampleVoiceBank_trigger(SampleVoiceBank *bank,uint8_t voice,uint8_t velocity);
void SampleVoiceBank_render(SampleVoiceBank *bank,uint8_t voice,int16_t *dst,uint32_t count);
#endif
