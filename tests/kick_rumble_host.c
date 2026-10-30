/* Host-side regression tests for experimental kick rumble DSP. */
#include "Soundengine/DSPAudio/KickRumble.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>

int main(void) {
    static KickRumble r;
    KickRumble_init(&r, 44100.0f);
    for (int i=0;i<1000;i++) {
        float dry = i==0 ? 0.35f : 0.0f;
        assert(KickRumble_process(&r,dry)==dry); /* factory bypass */
    }
    KickRumble_set(&r,44100.0f,0.75f,0.5f,35.f,160.f,0.75f);
    KickRumble_trigger(&r);
    double energy=0;
    for(int i=0;i<44100*3;i++){
        float v=KickRumble_process(&r,i==0 ? 0.3f : 0.0f);
        assert(isfinite(v));
        assert(v>=-1.0f && v<=1.0f);
        if (i>5000) energy += fabs(v);
    }
    assert(energy>0.00001);
    KickRumble_set(&r,44100.0f,0.0f,0.5f,35.f,160.f,0.75f);
    assert(KickRumble_process(&r,0.125f)==0.125f);
    puts("kick rumble host tests passed");
    return 0;
}
