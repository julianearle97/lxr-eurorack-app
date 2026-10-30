#include "Soundengine/IdmTriggers.h"
#include <assert.h>
#include <stdio.h>
static unsigned events=0; static unsigned voice_last=0;
static void emit(void *ctx,uint8_t voice,uint8_t note,uint8_t vel) {
    (void)ctx;(void)note;(void)vel;events++;voice_last=voice;
}
int main(void) {
    IdmEngine e; Idm_init(&e);
    Idm_input(&e,0,0,60,100,emit,0);
    assert(events==1 && voice_last==0); /* default transparent */
    e.voice[0]=(IdmSettings){IDM_BURST,100,1,127,1};
    events=0;Idm_input(&e,100,0,60,100,emit,0);
    for(unsigned t=100;t<300;t++)Idm_tick(&e,t,emit,0);
    assert(events==8);
    e.voice[0]=(IdmSettings){IDM_DELAY,100,1,127,1};
    events=0; Idm_input(&e,500,0,60,100,emit,0);
    assert(events==0);
    Idm_tick(&e,625,emit,0);
    assert(events==1);
    e.scope=IDM_MASTER;e.master=(IdmSettings){IDM_ROTATE,100,1,0,1};
    events=0;Idm_input(&e,1000,1,60,100,emit,0);
    assert(events==1 && voice_last==2);
    puts("IDM trigger engine host tests passed");
    return 0;
}
