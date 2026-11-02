#ifndef LXR_BBCUT_MONO_H
#define LXR_BBCUT_MONO_H
#include <stdint.h>
/* Experimental audio-cut engine: faithful Proc11 odd-unit phrase/block scheduling.
 * The original BBCut source is GPL-2.0; this derived port remains GPL-compatible. */
void BBCut_init(void);
void BBCut_setPpqn(uint8_t ppqn);
void BBCut_clockSample(float normalizedCv, uint32_t milliseconds);
void BBCut_clockPulse(uint32_t milliseconds);
uint8_t BBCut_clockActive(uint32_t milliseconds);
float BBCut_bpm(void);
void BBCut_processStereo(int16_t *output, uint32_t frames);
#endif
