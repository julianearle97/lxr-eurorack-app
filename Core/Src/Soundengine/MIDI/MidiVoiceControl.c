/*
 * MidiVoiceControl.c
 *
 *  Created on: 03.04.2012
 *
 * Copyright (C) 2012-2026 Erica Synths
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * This file is part of the LXR firmware. It is licensed under the GNU General
 * Public License version 3, with the additional permission and terms in
 * NOTICE.md. See LICENSE and NOTICE.md in the root of the repository.
 */


#include <Soundengine/TriggerProcessor.h>
#include "Soundengine/MIDI/MidiVoiceControl.h"
#include "Soundengine/IdmTriggers.h"
#include "Soundengine/SampleVoiceControl.h"
#include "menu/menu.h"
#include "Soundengine/DSPAudio/DrumVoice.h"
#include "Soundengine/DSPAudio/Snare.h"
#include "Soundengine/DSPAudio/HiHat.h"
#include "Soundengine/MIDI/MidiMessages.h"
#include "Soundengine/DSPAudio/CymbalVoice.h"
//#include "Soundengine/Sequencer/sequencer.h"
#include "Soundengine/MIDI/Uart.h"
#include "Soundengine/MIDI/MidiParser.h"
#include "main.h"
#include "ledHandler.h"
#include "buttonHandler.h"
//#include "LCD_driver.h"

uint8_t voiceStatus[NUM_DRUM_VOICES];
static IdmEngine idm;
static uint8_t idmInitialized=0;
static void idm_ensureInit(void) {if(!idmInitialized){Idm_init(&idm);idmInitialized=1;}}
static void idm_emit(void *ctx,uint8_t voice,uint8_t note,uint8_t vel) {
    (void)ctx;
    if(buttonHandler_getMutedVoices() & (1u<<voice)) return;
    /* Reconcile kit-loaded mode with voice state before every note-on. */
    menu_applySampleVoice(voice);
    SampleVoiceControl_trigger(voice,vel);
    if(SampleVoiceControl_isSample(voice)) {
       if(!buttonHandler_getDrumButton()) led_pulseLed(LED_2+voice);
       return;
    }
    if(voice<3) Drum_trigger(voice,vel,note);
    else if(voice==3) Snare_trigger(vel,note);
    else if(voice==4) Cymbal_trigger(vel,note);
    else if(voice<7) HiHat_trigger(vel,voice-5,note);
    if(!buttonHandler_getDrumButton()) led_pulseLed(LED_2+voice);
}
void voiceControl_idmSetVoice(uint8_t voice,uint8_t mode,uint8_t chance,uint8_t amount,uint8_t length) {
    idm_ensureInit();
    if(voice>=IDM_VOICES) return;
    idm.voice[voice]=(IdmSettings){mode,chance,length,amount,(uint8_t)(mode!=IDM_OFF)};
}
void voiceControl_idmSetMaster(uint8_t mode,uint8_t chance,uint8_t amount,uint8_t length,uint8_t mask) {
    idm_ensureInit();
    idm.master=(IdmSettings){mode,chance,length,amount,(uint8_t)(mode!=IDM_OFF)};
    idm.master_mask=mask & 0x7f;
}
void voiceControl_idmScope(uint8_t scope) {
    idm_ensureInit();
    idm.scope=(scope==2)? IDM_MASTER:IDM_INDEPENDENT;
    if(!scope) {
       for(unsigned i=0;i<IDM_VOICES;i++) idm.voice[i].enabled=0;
       idm.master.enabled=0;
    }
}


static uint8_t active_voices=0;	// which voices are currently playing a note
//----------------------------------------------------------------
static uint8_t voiceQueue[7] = { 0,0,0,0,0,0,0 };
static uint8_t voiceQueueNote[7] = { 0,0,0,0,0,0,0 };
static uint8_t voiceQueueVel[7] = { 0,0,0,0,0,0,0 };
uint8_t voiceControlStarted=0;

void voiceControl_queueTrigger(uint8_t voice, uint8_t note, uint8_t vel)
{
	if(voice<7)
	{
		voiceQueue[voice] = 1;
		voiceQueueNote[voice] = note;
		voiceQueueVel[voice] = vel;
	}
}

void voiceControl_processQueue()
{
	/* Dispatch previously scheduled glitch events on the audio processing thread. */
	idm_ensureInit();
	Idm_tick(&idm,HAL_GetTick(),idm_emit,0);
	if(voiceControlStarted)
	{
		for(int voice=0;voice<7;voice++)
		{

			if(voiceQueue[voice] == 1)
			{
				voiceQueue[voice] = 0;

				//only trigger unmuted voices
				if((buttonHandler_getMutedVoices()  & (1 << voice)) == 0)
				{

					if(voice<7) Idm_input(&idm,HAL_GetTick(),(uint8_t)voice,voiceQueueNote[voice],voiceQueueVel[voice],idm_emit,0);
                    else idm_emit(0,(uint8_t)voice,voiceQueueNote[voice],voiceQueueVel[voice]);
				}
			}
		}
	}
}
//----------------------------------------------------------------
void voiceControl_start()
{
	//clear all queued notes
	for(int voice=0;voice<7;voice++)
	{
		if(voiceQueue[voice] == 1)
		{
			voiceQueue[voice] = 0;
		}
	}
	//start queue processing
	voiceControlStarted = 1;
}
//----------------------------------------------------------------
// this fn assumes a valid voice is sent
void voiceControl_noteOn(uint8_t voice, uint8_t note, uint8_t vel)
{
	voiceControl_queueTrigger(voice, note, vel);
	/*
	if(voice < 3)
		//Drum_trigger(voice, vel, note);
	else if(voice < 4)
		Snare_trigger(vel, note);
	else if(voice < 5)
		Cymbal_trigger(vel, note);
	else
		HiHat_trigger(vel,voice-5,note);
	
	//only flash LEDs when not in voice select mode (DRUM)
	if(!buttonHandler_getDrumButton())
	{
		led_pulseLed(LED_2+voice);
	}
	*/
}
//----------------------------------------------------------------
void voiceControl_noteOff(uint8_t voice)
{
#if !EURORACK_MODULE
	uint8_t midiChan; // which midi channel to send a note on
#endif

	if(voice==0xff)
	{
		active_voices = 0;
#if !EURORACK_MODULE
		seq_midiNoteOff(0xff);
#endif
		return;
	}

	//only set voice inactive and send MIDI off when voice is currently playing
	if(active_voices & (1<<voice))
	{
		active_voices &= (~(1<<voice));
#if !EURORACK_MODULE
		//send midi note off
		midiChan = midi_MidiChannels[voice];

		seq_midiNoteOff(midiChan);
#endif
	}
}
//----------------------------------------------------------------
/*
uint8_t voiceControl_isVoicePlaying(uint8_t voice)
{
	return (active_voices & (1<<voice));
}
*/
