# Internal IDM-style trigger effects — development branch

Independent implementation from observed IDUM behavior; **no IDUM source code has been copied**. Mystic Circuits IDUM's published source uses CC BY-SA 4.0; licensing needs legal review before any reuse in LXR's GPL-3.0-only firmware.

## Target user experience

Select global OFF / INDEPENDENT / MASTER. Independent mode gives each of the six drum voices its own Mode, Chance, Length, Param page; Master applies one shared settings group to an opt-in six-voice mask. Keep Drum 1 unglitched by default. Existing four-knob rumble page and audio algorithm remain unchanged.

## Initial prototype (not connected to the LXR yet)

`Core/Src/Soundengine/IdmTriggers.c`: bounded 48-entry trigger event queue, deterministic bypass, probability, clock-derived burst, trigger divide, bouncing ball, trigger delay, voice rotation, and break stutters. Standalone host tests cover bypass, burst, delay, master rotate.

The following are **NOT IMPLEMENTED** in the firmware:
- UI controls and saved per-kit state
- integration in the hardware trigger IRQ and MIDI/note-on path
- an accurate shared clock pulse period and fine-grained timer
- effect Length state, eight-step pattern looper, IDUM Hold/gate durations, reverse/Skip clock behavior
- hat choke handling under rotated or delayed triggers
- exact IDUM timing and algorithm fidelity

## Safety/integration checklist

- Keep IRQs short: queue input events, never execute a whole ratchet in IRQ context.
- Scheduler tick needs sufficient resolution for fast ratchets (HAL millisecond tick may be insufficient).
- Make one dispatch path for external CV trigger and MIDI/sequencer note-on, with recursion prevention on emitted events.
- Handle queue overflow gracefully and avoid triggering outside the 6-voice hardware limits.
- Preserve original kit format or use a versioned extension to avoid breaking old kits.
- Benchmark peak CPU and audio continuity with full six-voice + V4 rumble.
- Disable all glitch effects by default, and provide one instant global bypass.

Do not flash this branch expecting trigger glitching yet. The V4 rumble firmware remains the last usable build.
