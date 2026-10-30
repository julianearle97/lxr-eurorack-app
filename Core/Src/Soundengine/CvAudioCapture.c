/* EXPERIMENTAL CV1 oscilloscope-like capture using existing ADC scan.
 * Keep the 13 channel scan unchanged: CV1 is conversion index 8.
 * Half/full DMA interrupts carry 16 new CV1 samples to a ring.
 */
#include "Soundengine/CvAudioCapture.h"
#include "IO/adcPots.h"
#include <string.h>
#define CAP_RING 2048u
uint16_t cv_adc_dma[CV_ADC_DMA_SAMPLES];
static uint16_t ring[CAP_RING];
static volatile uint32_t write_pos=0;
static uint32_t read_pos=0;
static float last_sample=2048.0f;
void cv_audio_capture_half(uint8_t half){
 const uint32_t start=half ? CV_ADC_DMA_SAMPLES/2 : 0;
 /* Preserve existing knob/CV control readings for original firmware. */
 const uint32_t final=start+(CV_ADC_SCANS_PER_DMA/2-1)*CV_ADC_SCAN_CHANNELS;
 for(uint32_t k=0;k<CV_ADC_SCAN_CHANNELS;k++) adc_buf[k]=cv_adc_dma[final+k];
 for(uint32_t j=0;j<CV_ADC_SCANS_PER_DMA/2;j++){
   ring[write_pos&(CAP_RING-1)]=cv_adc_dma[start+j*CV_ADC_SCAN_CHANNELS+8]>>4;
   write_pos++;
 }
}
float cv_audio_capture_read(void){
 uint32_t w=write_pos;
 uint32_t available=w-read_pos;
 /* Target a small queue (~0.5ms). Correct clock drift, never block. */
 if(available>CAP_RING-32){read_pos=w-32;available=32;}
 if(available>48){read_pos+=available-24;available=24;}
 if(available>1){
   uint16_t raw=ring[read_pos&(CAP_RING-1)];
   read_pos++;
   last_sample=(float)raw;
 }
 return last_sample;
}
