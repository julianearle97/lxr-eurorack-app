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
    /* Decay must span from nearly immediate to a sustained tail. */
    KickRumble_set(&r,44100.0f,0.7f,0.0f,25.f,180.f,0.0f);
    const float shortFeedback = r.feedback;
    KickRumble_set(&r,44100.0f,0.7f,1.0f,25.f,180.f,0.0f);
    const float longFeedback = r.feedback;
    assert(shortFeedback < 0.1f);
    assert(longFeedback > 0.9f);
    assert(longFeedback > shortFeedback);
    /* Kick trigger must actually reduce the wet envelope for a while,
       then return to unity without lowering the dry voice. */
    KickRumble_set(&r,44100.0f,0.7f,0.7f,25.f,180.f,1.0f);
    KickRumble_trigger(&r);
    for(int i=0;i<1000;i++) (void)KickRumble_process(&r,0.0f);
    assert(r.duck_gain < 0.05f);
    for(int i=0;i<15000;i++) (void)KickRumble_process(&r,0.0f);
    assert(r.duck_gain > 0.75f);
    KickRumble_set(&r,44100.0f,0.7f,0.7f,25.f,180.f,0.0f);
    assert(r.duck_gain == 1.0f);
    puts("kick rumble host tests passed");
    return 0;
}
