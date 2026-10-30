/* Experimental asynchronous CV1 ADC -> audio clock recovery.
 * Consumer continuously adjusts its source step to match the ADC producer.
 * Linear fractional interpolation eliminates the hard two-sample decimation.
 */
#include "Soundengine/Cv1DedicatedCapture.h"
#define CV1_RING_SIZE 4096u
#define CV1_MASK (CV1_RING_SIZE-1u)
uint16_t cv1_dma_samples[CV1_DMA_COUNT];
static uint16_t cv1_ring[CV1_RING_SIZE];
static volatile uint32_t head;
static uint32_t tail;
static uint32_t phase_q16;
static uint32_t step_q16=131072u;
static float previous=2048.0f;
void cv1_capture_dma_half(unsigned half){
    unsigned start=half?CV1_DMA_COUNT/2:0;
    for(unsigned i=0;i<CV1_DMA_COUNT/2;i++){
        cv1_ring[head&CV1_MASK]=(uint16_t)(cv1_dma_samples[start+i]>>4);
        head++;
    }
}
float cv1_capture_next(void){
    uint32_t w=head;
    uint32_t available=w-tail;
    /* Reset only after genuine buffer overrun; avoid discontinuities in normal operation. */
    if(available>=CV1_RING_SIZE-8u){
        tail=w-256u;
        phase_q16=0;
        available=256u;
    }
    /* Track producer rate using a very slow queue-depth servo. A stable
     * queue is more important than guessing the ADC conversion rate. */
    const int32_t error=(int32_t)available-192;
    int32_t goal=131072+(error*64);
    if(goal<32768)goal=32768;
    if(goal>1048576)goal=1048576;
    step_q16=(uint32_t)((int32_t)step_q16+(goal-(int32_t)step_q16)/256);
    if(available>=2u){
        const int32_t a=cv1_ring[tail&CV1_MASK];
        const int32_t b=cv1_ring[(tail+1u)&CV1_MASK];
        previous=(float)a + (float)(b-a)*(float)phase_q16/65536.0f;
        phase_q16+=step_q16;
        uint32_t advance=phase_q16>>16;
        if(advance>available-2u) {advance=available-2u;phase_q16=advance<<16;}
        tail+=advance;
        phase_q16-=advance<<16;
    }
    return previous;
}
