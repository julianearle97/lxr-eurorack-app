/* Independent implementation of internal trigger glitch scheduling.
 * No Mystic Circuits IDUM code copied. GPL-3.0-only.
 * Experimental engine; not yet connected to firmware's trigger paths/UI.
 */
#ifndef LXR_IDM_TRIGGERS_H
#define LXR_IDM_TRIGGERS_H
#include <stdint.h>
#define IDM_VOICES 7
#define IDM_EVENTS 48
typedef enum { IDM_OFF, IDM_BURST, IDM_DIVIDE, IDM_BALL, IDM_DELAY, IDM_ROTATE, IDM_BREAK } IdmMode;
typedef enum { IDM_INDEPENDENT, IDM_MASTER } IdmScope;
typedef struct {
    uint8_t mode, chance, length, param, enabled;
} IdmSettings;
typedef struct {uint32_t due; uint8_t voice, velocity, note, used;} IdmEvent;
typedef struct {
    IdmSettings voice[IDM_VOICES];
    IdmSettings master;
    IdmScope scope;
    uint8_t master_mask;
    uint32_t step_ms, seed, last_trigger[IDM_VOICES];
    uint8_t div_counter[IDM_VOICES];
    IdmEvent events[IDM_EVENTS];
} IdmEngine;
typedef void (*IdmEmit)(void *context,uint8_t voice,uint8_t note,uint8_t velocity);
void Idm_init(IdmEngine *engine);
void Idm_input(IdmEngine *engine,uint32_t now_ms,uint8_t voice,uint8_t note,uint8_t velocity,IdmEmit emit,void *ctx);
void Idm_tick(IdmEngine *engine,uint32_t now_ms,IdmEmit emit,void *ctx);
#endif
