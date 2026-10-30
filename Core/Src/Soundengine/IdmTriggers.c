#include "Soundengine/IdmTriggers.h"
#include <string.h>
static uint32_t rnd(IdmEngine *e) {
    uint32_t x=e->seed; x^=x<<13; x^=x>>17; x^=x<<5; e->seed=x; return x;
}
static void schedule(IdmEngine *e,uint32_t due,uint8_t v,uint8_t n,uint8_t vel) {
    for(unsigned i=0;i<IDM_EVENTS;i++) if(!e->events[i].used){
        e->events[i]=(IdmEvent){due,v,vel,n,1};return;
    }
    /* Full queue deliberately drops extra ratchets; bounded real-time workload. */
}
void Idm_init(IdmEngine *e) {
    memset(e,0,sizeof(*e));
    e->seed=0x9e3779b9u; e->step_ms=125;
    e->master_mask=0x7f; e->scope=IDM_INDEPENDENT;
}
void Idm_input(IdmEngine *e,uint32_t now,uint8_t voice,uint8_t note,uint8_t vel,IdmEmit emit,void *ctx) {
    if(voice>=IDM_VOICES) return;
    IdmSettings *s=&e->voice[voice];
    if(e->scope==IDM_MASTER && (e->master_mask&(1u<<voice))) s=&e->master;
    if(!s->enabled || s->mode==IDM_OFF || (rnd(e)%100u)>=s->chance) {
        emit(ctx,voice,note,vel);return;
    }
    const uint32_t step=e->step_ms?e->step_ms:125u;
    const uint32_t interval=step/(2u+(s->param*6u)/127u);
    switch(s->mode) {
      case IDM_BURST: {
        unsigned count=2u+(s->param*6u)/127u;
        for(unsigned i=0;i<count;i++) schedule(e,now+i*(interval?interval:1),voice,note,vel);
        break;
      }
      case IDM_DIVIDE: {
        unsigned divisor=2u+(s->param*6u)/127u;
        if((e->div_counter[voice]++%divisor)==0) emit(ctx,voice,note,vel);
        break;
      }
      case IDM_BALL: {
        unsigned n=2u+(s->param*5u)/127u;
        uint32_t t=now,dt=step/2u;
        for(unsigned i=0;i<n;i++){schedule(e,t,voice,note,vel);t+=dt?dt:1;dt=dt*3u/4u;}
        break;
      }
      case IDM_DELAY: schedule(e,now+1u+(step*s->param)/127u,voice,note,vel);break;
      case IDM_ROTATE: emit(ctx,(uint8_t)((voice+1+(s->param*4u)/127u)%IDM_VOICES),note,vel);break;
      case IDM_BREAK: {
        emit(ctx,voice,note,vel);
        if(s->param>32) schedule(e,now+step/2u,voice,note,vel);
        if(s->param>80) schedule(e,now+step*3u/4u,voice,note,vel);
        break;
      }
      default:emit(ctx,voice,note,vel);break;
    }
    e->last_trigger[voice]=now;
}
void Idm_tick(IdmEngine *e,uint32_t now,IdmEmit emit,void *ctx) {
    for(unsigned i=0;i<IDM_EVENTS;i++) {
        IdmEvent *event=&e->events[i];
        if(event->used && (int32_t)(now-event->due)>=0) {
            uint8_t v=event->voice,n=event->note,vel=event->velocity;
            event->used=0;
            emit(ctx,v,n,vel);
        }
    }
}
