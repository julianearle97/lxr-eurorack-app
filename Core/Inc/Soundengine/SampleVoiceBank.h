/* Experimental mono PCM8 one-shot engine, GPL-3.0-only.
 * Host-tested standalone; NOT yet integrated into LXR firmware.
 */
#ifndef LXR_SAMPLE_VOICE_H
#define LXR_SAMPLE_VOICE_H
#include <stdint.h>
#define SAMPLE_VOICE_COUNT 7
#define SAMPLE_SLOT_COUNT 12
typedef struct { const int8_t *data; uint32_t length; } SampleSlot;
typedef struct {
    uint64_t phase_q16;
    uint32_t increment_q16;
    uint8_t sample_id, enabled, volume, velocity;
} SampleVoiceState;
typedef struct {
    SampleVoiceState voices[SAMPLE_VOICE_COUNT];
    SampleSlot slots[SAMPLE_SLOT_COUNT];
} SampleVoiceBank;
void SampleVoiceBank_init(SampleVoiceBank *bank);
void SampleVoiceBank_assign(SampleVoiceBank *bank,uint8_t sample_id,const int8_t *data,uint32_t length);
void SampleVoiceBank_setVoice(SampleVoiceBank *bank,uint8_t voice,uint8_t enabled,
                              uint8_t sample_id,uint8_t pitch,uint8_t volume);
void SampleVoiceBank_trigger(SampleVoiceBank *bank,uint8_t voice,uint8_t velocity);
void SampleVoiceBank_stop(SampleVoiceBank *bank,uint8_t voice);
void SampleVoiceBank_render(SampleVoiceBank *bank,uint8_t voice,int16_t *dst,uint32_t count);
#endif
