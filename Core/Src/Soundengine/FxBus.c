/*
 * FxBus.c
 *
 *  Created on: 26.08.2020
 *      Author: julian
 *
 * Copyright (C) 2012-2026 Erica Synths
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * This file is part of the LXR firmware. It is licensed under the GNU General
 * Public License version 3, with the additional permission and terms in
 * NOTICE.md. See LICENSE and NOTICE.md in the root of the repository.
 */


#include "Soundengine/FxBus.h"
#include "Soundengine/BBCutMono.h"
#include "stm32f7xx_hal.h"
#include "Soundengine/DSPAudio/BufferTools.h"
#include "Soundengine/DSPAudio/mixer.h"
INDTCMZ FxBus fxBus;

void FxBus_init()
{
	fxBus.dryWet = 0;
	fxBus.audioRouting = 0;
	fxBus.fxType = FX_OFF;

	fxBus.delay.delayType = DELAY_NORMAL;

	Distortion_init(&fxBus.distortion);
	Ringmod_init(&fxBus.ringmod);
	Compressor_init(&fxBus.compressor);
	Delay_init(&fxBus.delay);



#if USE_REVERB
	Reverb_init(&fxBus.reverb);
#endif
}

IN_ITCM_RAM void FxBus_process(int16_t *fxBuffer, int16_t* outL,int16_t* outR,int16_t* outL2, int16_t* outR2, uint32_t size)
{
	int16_t* out;
	int16_t* out2 = NULL;
	// < MIXER_ROUTING_DAC1_L -> stereo 1 or 2
	const uint8_t channels = fxBus.audioRouting < MIXER_ROUTING_DAC1_L ? 2 : 1;
	switch(fxBus.audioRouting)
	{
	default:
	case MIXER_ROUTING_DAC1_STEREO:
		out = outL2;
		out2 = outR2;
		break;
	case MIXER_ROUTING_DAC2_STEREO:
		out = outL;
		out2 = outR;
		break;
	case MIXER_ROUTING_DAC1_L:
		out = outL2;
		break;
	case MIXER_ROUTING_DAC1_R:
		out = outR2;
		break;
	case MIXER_ROUTING_DAC2_L:
		out = outL;
		break;
	case MIXER_ROUTING_DAC2_R:
		out = outR;
		break;

	}

	switch(fxBus.fxType)
	{
	default:
	case FX_OFF:
		{
			if(channels == 2)
			{
				int32_t* stereoIn = (int32_t*)fxBuffer;
				int32_t* stereoOut = (int32_t*)out;

				for(uint8_t i=0;i<size;i++)
				{
					//add stereo samples using SIMD
					*stereoOut = __QADD16(*stereoOut, *stereoIn);
					stereoOut++;
					stereoIn++;
				}
			}
			else
			{
				int32_t* in = (int32_t*)fxBuffer;
				for(uint8_t i=0;i<size;i++)
				{
					*out =  __QADD16(*out, *in);
					out +=2;
					in++;

				}
			}
		}
		break;

	case FX_DISTORTION:
		Distortion_process(&fxBus.distortion, fxBuffer, out, out2, size, channels, fxBus.dryWet);
		break;

	case FX_RING:
		Ringmod_process(&fxBus.ringmod, fxBuffer, out, out2, size, channels, fxBus.dryWet);
		break;

	case FX_COMPRESSOR:
		Compressor_process(&fxBus.compressor, fxBuffer, out, out2, size, channels, fxBus.dryWet);
		break;

	case FX_DELAY:
        {
            /* Musical note division relative to incoming ACC1 tempo.
             * The built-in delay RAM is only 32768 samples (0.743 s).
             * Requested longer divisions are clamped to its max capacity. */
            static uint8_t previousDiv=255u;
            static float previousBpm=0.f;
            static uint8_t wasExternallyClocked=0u;
            uint8_t external=BBCut_clockActive(HAL_GetTick());
            float bpm=external?BBCut_bpm():120.f;
            if(bpm<20.f)bpm=120.f;
            uint8_t div=(uint8_t)fxBus.parameter[0];
            if(div>5u)div=5u;
            const float beatMultipliers[6]={4.f,2.f,1.f,0.5f,0.25f,0.125f};
            if(div!=previousDiv || (bpm>previousBpm+0.25f) ||
               (bpm<previousBpm-0.25f) || external!=wasExternallyClocked) {
                float samples=(44100.f*60.f*beatMultipliers[div])/bpm;
                if(samples>32760.f)samples=32760.f;
                if(samples<900.f)samples=900.f;
                float normalized=(samples-882.f)/(32767.f-882.f);
                if(normalized<0.f)normalized=0.f;
                if(normalized>1.f)normalized=1.f;
                Delay_setRange(&fxBus.delay,1.f);
                Delay_setDelayTime(&fxBus.delay,normalized);
                previousDiv=div;previousBpm=bpm;wasExternallyClocked=external;
            }
        }
        if(fxBus.delay.delayType == DELAY_NORMAL)
		{
			Delay_process(&fxBus.delay, fxBuffer, out, out2, size, channels, fxBus.dryWet);
		}
		else
		{
			Delay_processPingPong(&fxBus.delay, fxBuffer, out, out2, size, channels, fxBus.dryWet);
		}
		break;
#if USE_REVERB
	case FX_REVERB:
		Reverb_process(&fxBus.reverb, fxBuffer, out, out2, size, channels, fxBus.dryWet);
		break;
#endif

	}

}

void FxBus_prepareFxChange()
{
	//memset(FX_BUFFER, 0, FX_BUFFER_SIZE*sizeof(__fp16) );
//	Ringmod_resetPhase(&fxBus.ringmod);
}

IN_ITCM_RAM void FxBus_updateParameter()
{
	switch(fxBus.fxType)
	{
	case FX_OFF:

		break;

	case FX_DISTORTION:
		Distortion_setDrive(&fxBus.distortion, fxBus.parameter[0] );
		Distortion_setParam1(&fxBus.distortion, fxBus.parameter[1] );
		Distortion_setParam2(&fxBus.distortion, fxBus.parameter[2] );
		Distortion_setParam3(&fxBus.distortion, fxBus.parameter[3] );
		break;

	case FX_RING:
		Ringmod_setFreq(&fxBus.ringmod, fxBus.parameter[0] );
		//Ringmod_setWave(&fxBus.ringmod, fxBus.parameter[1] );
		break;

	case FX_COMPRESSOR:

		Compressor_setAttack(&fxBus.compressor , fxBus.parameter[0]);
		Compressor_setRelease(&fxBus.compressor , fxBus.parameter[1]);
		Compressor_setTreshold(&fxBus.compressor , fxBus.parameter[2]);
		Compressor_setGain(&fxBus.compressor , fxBus.parameter[3]);
		break;

	case FX_DELAY:
        /* Clock-controlled time updated in FxBus_process, not on knob refresh. */
		Delay_setFilter(&fxBus.delay, fxBus.parameter[2]);
		Delay_setFeedback(&fxBus.delay, fxBus.parameter[3]);

		break;
#if USE_REVERB
	case FX_REVERB:
		Reverb_setDelayTime(&fxBus.reverb, fxBus.parameter[0]);
		Reverb_setFeedback(&fxBus.reverb, fxBus.parameter[1]);
		Reverb_setComposition(&fxBus.reverb, fxBus.parameter[2]);

		Reverb_setVariation(&fxBus.reverb, fxBus.parameter[3]);
		break;
#endif
	default:
		break;
	}
}

IN_ITCM_RAM void FxBus_setParameter(uint8_t idx, float value)
{
	fxBus.parameter[idx] = value;

	FxBus_updateParameter();


}


