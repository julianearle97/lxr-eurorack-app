#include "Soundengine/BBCutMono.h"
#include <string.h>
/* Mono rolling audio buffer: ring size is power of two.
 * The buffer is deliberately bounded, unlike SuperCollider's allocated buffers. */
#define BBCUT_RING 16384u
#define BBCUT_MASK (BBCUT_RING-1u)
#define BBCUT_FS 44100u
static int16_t ring[BBCUT_RING];
static uint32_t writePos, sampleCount, cutRemaining, cutLength, cutRead;
static uint32_t repeatsRemaining, phraseUnitsDone, heldStart, cycle;
static uint8_t playbackMode, clockPulseCount;
static volatile uint8_t barSyncPending;
static uint8_t ppqn=1, gate, validClock, stutter;
static uint32_t lastRiseMs, pulseMs, seed=0x12a74b19u;
static float tempo=120.f;
static uint8_t bbEnabled=0,bbChance=50,bbRepeat=64,bbSpeed=64;
void BBCut_setControls(uint8_t on,uint8_t chance,uint8_t repeat,uint8_t speed){bbEnabled=(on!=0);bbChance=chance;bbRepeat=repeat;bbSpeed=speed;}
uint8_t BBCut_enabled(void){return bbEnabled;}
static uint32_t rng(void) {seed^=seed<<13;seed^=seed>>17;seed^=seed<<5;return seed;}
static uint32_t clampSample(uint32_t n) {if(n<16u)return 16u; if(n>BBCUT_RING/2u)return BBCUT_RING/2u; return n;}
void BBCut_init(void) {
  memset(ring,0,sizeof(ring));
  writePos=sampleCount=cutRemaining=cutLength=cutRead=repeatsRemaining=phraseUnitsDone=0;
  lastRiseMs=pulseMs=0; gate=validClock=stutter=0; tempo=120.f; ppqn=1;
  heldStart=cycle=clockPulseCount=0; playbackMode=0; barSyncPending=0;
}
void BBCut_setPpqn(uint8_t n) {
  if(n==1 || n==2 || n==4 || n==8 || n==16 || n==24) {
    if(ppqn!=n){ppqn=n; validClock=0; lastRiseMs=0;}
  }
}
void BBCut_clockPulse(uint32_t now) {
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
    clockPulseCount++;
    /* Re-anchor phrase scheduling to the external beat boundary. */
    if(clockPulseCount>=ppqn){clockPulseCount=0;cycle++;if((cycle&3u)==0u)barSyncPending=1u;}
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
/* BBCutProc11-derived phrase scheduler, constrained to a short ring buffer.
 * A choice of live, backward repeat, half-speed and reverse prevents the
 * previous single-loop implementation from endlessly cycling one fragment.
 * Only recently recorded material is accessed. */
static void nextBlock(void) {
  if(phraseUnitsDone>=24u) phraseUnitsDone=0;
  uint32_t left=24u-phraseUnitsDone;
  stutter=((rng()%100u)<bbChance) && (left<=8u || bbChance>=75u);
  uint32_t unit=samplesPerUnit();
  if(stutter) {
    cutLength=clampSample(unit/(2u+bbSpeed/24u));
    repeatsRemaining=2u+(rng()%4u);
    phraseUnitsDone=24u;
  } else {
    uint32_t odd=1u+2u*(rng()%(bbSpeed>90u?2u:3u));
    if(odd>left) odd=left;
    cutLength=clampSample(unit*odd);
    repeatsRemaining=1u+((rng()%128u)<bbRepeat ? 1u+(rng()%3u):0u);
    phraseUnitsDone+=odd*repeatsRemaining;
    if(phraseUnitsDone>24u) phraseUnitsDone=24u;
  }
  playbackMode=(uint8_t)((rng()%100u)<bbChance ? (1u+(rng()%4u)):0u);
  heldStart=(writePos+BBCUT_RING-cutLength)&BBCUT_MASK;
  cutRemaining=0;
}
void BBCut_processStereo(int16_t *out,uint32_t frames) {
  for(uint32_t i=0;i<frames;i++) {
    if(barSyncPending){barSyncPending=0;phraseUnitsDone=0;repeatsRemaining=0;cutRemaining=0;}
    int32_t sum=(int32_t)out[i*2]+out[i*2+1];
    int16_t mono=(int16_t)(sum/2);
    ring[writePos]=mono;
    if(!repeatsRemaining && !cutRemaining) nextBlock();
    if(!cutRemaining) {
      cutRemaining=cutLength;
      if(repeatsRemaining)repeatsRemaining--;
      cutRead=heldStart;
      /* Nonrepeat blocks can pass through; other blocks pull from history. */
      if(playbackMode==0u) cutRead=writePos;
      else if(playbackMode==3u) cutRead=(writePos+BBCUT_RING-(cutLength/2u))&BBCUT_MASK;
      else if(playbackMode==4u) cutRead=(writePos+BBCUT_RING-(rng()% (cutLength+1u)))&BBCUT_MASK;
    }
    uint32_t progress=cutLength-cutRemaining;
    uint32_t rd=cutRead;
    if(playbackMode==1u)rd=(cutRead+cutLength-1u-progress)&BBCUT_MASK;
    else if(playbackMode==2u)rd=(cutRead+(progress>>1))&BBCUT_MASK;
    else rd=(cutRead+progress)&BBCUT_MASK;
    int16_t result=(sampleCount>256u)?ring[rd]:mono;
    /* Short linear crossfade into cut edges to reduce clicks. */
    uint32_t fade=progress<32u?progress: (cutRemaining<32u?cutRemaining:32u);
    if(fade<32u)result=(int16_t)(((int32_t)result*(int32_t)fade+(int32_t)mono*(int32_t)(32u-fade))/32);
    out[i*2]=result;out[i*2+1]=result;
    writePos=(writePos+1u)&BBCUT_MASK;
    if(sampleCount<BBCUT_RING)sampleCount++;
    cutRemaining--;
  }
}
