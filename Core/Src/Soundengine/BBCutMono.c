#include "Soundengine/BBCutMono.h"
#include <string.h>
/* Mono rolling audio buffer: ring size is power of two.
 * The buffer is deliberately bounded, unlike SuperCollider's allocated buffers. */
#define BBCUT_RING 16384u
#define BBCUT_MASK (BBCUT_RING-1u)
#define BBCUT_FS 44100u
static int16_t ring[BBCUT_RING];
static uint32_t writePos, sampleCount, cutRemaining, cutLength, cutRead;
static uint32_t repeatsRemaining, phraseUnitsDone;
static uint8_t ppqn=1, gate, validClock, stutter;
static uint32_t lastRiseMs, pulseMs, seed=0x12a74b19u;
static float tempo=120.f;
static uint32_t rng(void) {seed^=seed<<13;seed^=seed>>17;seed^=seed<<5;return seed;}
static uint32_t clampSample(uint32_t n) {if(n<16u)return 16u; if(n>BBCUT_RING-64u)return BBCUT_RING-64u; return n;}
void BBCut_init(void) {
  memset(ring,0,sizeof(ring));
  writePos=sampleCount=cutRemaining=cutLength=cutRead=repeatsRemaining=phraseUnitsDone=0;
  lastRiseMs=pulseMs=0; gate=validClock=stutter=0; tempo=120.f; ppqn=1;
}
void BBCut_setPpqn(uint8_t n) {
  if(n==1 || n==2 || n==4 || n==8 || n==16 || n==24) {
    if(ppqn!=n){ppqn=n; validClock=0; lastRiseMs=0;}
  }
}
void BBCut_clockSample(float cv, uint32_t now) {
  if(!gate && cv>0.60f) {
    gate=1;
    if(lastRiseMs) {
      uint32_t dt=now-lastRiseMs;
      if(dt>=3u && dt<4000u) {
        float next=60000.f/((float)dt*(float)ppqn);
        if(next>=20.f && next<=300.f) {
          pulseMs=dt;
          tempo=validClock?tempo*0.75f+next*0.25f:next;
          validClock=1;
        }
      }
    }
    lastRiseMs=now;
  } else if(gate && cv<0.35f) gate=0;
}
uint8_t BBCut_clockActive(uint32_t now) {
  if(!validClock || !pulseMs) return 0;
  uint32_t timeout=pulseMs*3u;
  if(timeout<500u)timeout=500u;
  if(timeout>5000u)timeout=5000u;
  if(now-lastRiseMs>timeout) { validClock=0; return 0; }
  return 1;
}
float BBCut_bpm(void) {return tempo;}
static uint32_t samplesPerUnit(void) {
  /* Proc11 default sdiv=8, barlength=4 => half-beat cut units. */
  return clampSample((uint32_t)(BBCUT_FS*30.f/tempo));
}
static void nextBlock(void) {
  /* BBCutProc11: phrases of 3 bars, 8 subdivision units/bar;
   * normal blocks have odd lengths and 1-2 repeats;
   * stutters are allowed in the final half-bar. */
  if(phraseUnitsDone>=24u)phraseUnitsDone=0;
  uint32_t left=24u-phraseUnitsDone;
  stutter=(left<=4u && (rng()%10u)<2u);
  uint32_t units=1u, reps=1u;
  if(stutter) {
    units=1u; reps=left*2u;
    cutLength=clampSample(samplesPerUnit()/2u);
    phraseUnitsDone=24u;
  } else {
    uint32_t candidate=(rng()%2u)*2u+1u; /* 1 or 3 units */
    units=candidate>left?left:candidate;
    reps=1u+(rng()&1u);
    if(reps*units>left){reps=1u;units=left;}
    cutLength=clampSample(samplesPerUnit()*units);
    phraseUnitsDone+=reps*units;
  }
  repeatsRemaining=reps;
  cutRemaining=0u;
}
void BBCut_processStereo(int16_t *out,uint32_t frames) {
  for(uint32_t i=0;i<frames;i++) {
    int32_t a=out[2*i],b=out[2*i+1];
    int16_t mono=(int16_t)((a+b)/2);
    ring[writePos&BBCUT_MASK]=mono;
    uint16_t ready=(sampleCount>cutLength+64u);
    if(!repeatsRemaining) nextBlock();
    if(!cutRemaining) {
      if(repeatsRemaining>0u)repeatsRemaining--;
      /* Repeat a previous chunk, never reading unwritten audio. */
      cutRead=(writePos+BBCUT_RING-cutLength)&BBCUT_MASK;
      cutRemaining=cutLength;
    }
    int16_t v=ready?ring[cutRead&BBCUT_MASK]:mono;
    if(ready)cutRead=(cutRead+1u)&BBCUT_MASK;
    out[2*i]=v;out[2*i+1]=v;
    writePos=(writePos+1u)&BBCUT_MASK;
    if(sampleCount<BBCUT_RING)sampleCount++;
    cutRemaining--;
  }
}
