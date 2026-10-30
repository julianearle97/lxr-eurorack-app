#ifndef LXR_CV_SIDECHAIN_EXPERIMENT_H
#define LXR_CV_SIDECHAIN_EXPERIMENT_H
#include <stdint.h>
void cv_sidechain_kick(void);
void cv_sidechain_set(uint8_t duck,uint8_t hp,uint8_t gain,uint8_t release);
void cv_sidechain_mix(int16_t *output3_interleaved,uint32_t frames);
#endif
