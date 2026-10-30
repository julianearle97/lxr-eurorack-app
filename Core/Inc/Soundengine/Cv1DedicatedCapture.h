#ifndef CV1_DEDICATED_CAPTURE_H
#define CV1_DEDICATED_CAPTURE_H
#include <stdint.h>
#define CV1_DMA_COUNT 512u
extern uint16_t cv1_dma_samples[CV1_DMA_COUNT];
void cv1_capture_dma_half(unsigned half);
float cv1_capture_next(void);
#endif
