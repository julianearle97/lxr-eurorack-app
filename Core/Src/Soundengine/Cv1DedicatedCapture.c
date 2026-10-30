/* Deliberately experimental: ADC1 assigned solely to CV1. */
#include "Soundengine/Cv1DedicatedCapture.h"
#define CV1_RING_SIZE 4096u
uint16_t cv1_dma_samples[CV1_DMA_COUNT];
static uint16_t cv1_ring[CV1_RING_SIZE];
static volatile uint32_t head;
static uint32_t tail;
static float previous=2048.0f;
void cv1_capture_dma_half(unsigned half){
    unsigned start=half?CV1_DMA_COUNT/2:0;
    for(unsigned i=0;i<CV1_DMA_COUNT/2;i++){
        cv1_ring[head&(CV1_RING_SIZE-1)]=(uint16_t)(cv1_dma_samples[start+i]>>4);
        head++;
    }
}
/* Approximately two ADC samples per codec sample at 86 kS/s ADC scan.
 * Average consecutive samples. Since ADC is free-running, clock drift can
 * cause discontinuities: this is a bandwidth experiment, not hi-fi I/O. */
float cv1_capture_next(void){
    uint32_t n=head-tail;
    if(n>CV1_RING_SIZE-512u){tail=head-512u;n=512;}
    if(n>768u){tail=head-512u;n=512;}
    if(n>=2u){
       uint16_t a=cv1_ring[tail&(CV1_RING_SIZE-1)];
       uint16_t b=cv1_ring[(tail+1u)&(CV1_RING_SIZE-1)];
       tail+=2;
       previous=0.5f*((float)a+(float)b);
    }
    return previous;
}
