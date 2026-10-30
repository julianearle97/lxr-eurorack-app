#include "Soundengine/SampleVoiceBank.h"
#include <assert.h>
#include <stdio.h>
static const int8_t s1[]={0,32,64,96,127,96,64,32,0};
static const int8_t s2[]={127,0,-127};
int main(void){
 SampleVoiceBank b; SampleVoiceBank_init(&b);
 SampleVoiceBank_assign(&b,0,s1,sizeof(s1));
 SampleVoiceBank_assign(&b,1,s2,sizeof(s2));
 SampleVoiceBank_setVoice(&b,0,1,0,64,127);
 int16_t buf[32];
 SampleVoiceBank_trigger(&b,0,127);
 SampleVoiceBank_render(&b,0,buf,9);
 assert(buf[4]>30000 && buf[8]==0);
 SampleVoiceBank_render(&b,0,buf,20);
 for(unsigned i=0;i<20;i++)assert(buf[i]==0);
 SampleVoiceBank_setVoice(&b,0,1,1,64,64);
 SampleVoiceBank_trigger(&b,0,127);
 SampleVoiceBank_render(&b,0,buf,1);
 assert(buf[0]>12000 && buf[0]<17000);
 SampleVoiceBank_setVoice(&b,1,1,0,127,127);
 SampleVoiceBank_trigger(&b,1,127);
 SampleVoiceBank_render(&b,1,buf,6);
 assert(buf[1]>0);
 SampleVoiceBank_stop(&b,1);
 SampleVoiceBank_render(&b,1,buf,4);
 for(int i=0;i<4;i++)assert(buf[i]==0);
 puts("sample bank pitch-volume host tests passed");
}
