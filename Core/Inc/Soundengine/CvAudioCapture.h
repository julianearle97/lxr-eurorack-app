#ifndef LXR_CV_AUDIO_CAPTURE_H
#define LXR_CV_AUDIO_CAPTURE_H
#include <stdint.h>
#define CV_ADC_SCAN_CHANNELS 13u
#define CV_ADC_SCANS_PER_DMA 32u
#define CV_ADC_DMA_SAMPLES (CV_ADC_SCAN_CHANNELS*CV_ADC_SCANS_PER_DMA)
extern uint16_t cv_adc_dma[CV_ADC_DMA_SAMPLES];
void cv_audio_capture_half(uint8_t half);
float cv_audio_capture_read(void);
#endif
