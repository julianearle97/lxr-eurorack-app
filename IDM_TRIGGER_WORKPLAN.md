# Experimental internal IDM-style triggers — v1.55.0

This is an **independent approximation** of gate/trigger mutation effects, **not** the exact Mystic Circuits IDUM firmware. No CC BY-SA source has been copied into the GPL LXR codebase.

## Controls

In each Drum 1–7 Mix screen, scroll beyond the existing controls to the four-knob glitch page.

- **Mode**: 0=Off, 1=Burst, 2=Divide, 3=Ball, 4=Delay, 5=Rotate, 6=Break.
- **Chance**: probability that incoming trigger is transformed, 0–100%.
- **Amount**: timing, speed, division or rotation, depending on mode.
- **Length**: burst/bounce count, delay interval multiplier or break accents.

Drum 1 preserves the existing rumble page between Mix and Glitch. The standalone Perform page has four master controls with the same mapping. Master mode **Off** enables independent modes. Master mode 1–6 overrides Drum 2–7 as a group, deliberately excluding Drum 1; Drum 1 stays independent.

## Implementation

A bounded 48-event non-blocking scheduler runs in the existing voice audio dispatch thread. Both physical triggers routed through the LXR queue and MIDI/sequence notes flow into the same trigger path. The separate open-hat note is treated as Drum 7 and retains the module's original HiHat_trigger choke logic when dispatched.

The master and seven independent four-byte settings groups are preserved per kit in a versioned optional `IDM\x01` extension after the existing `RMB\x01` custom rumble extension. Factory kits load with glitch modes OFF. A fully disabled glitch mode passes events directly.

## Known differences from physical IDUM

- This build does **not** reproduce IDUM's source algorithms, eight-step looper, Hold/gate-duration, or clock-output Skip controls.
- Effects operate on internal note events, not external gate outputs or physical clock lines.
- Tick timing currently uses the 1 ms system clock and audio-block dispatcher; extremely fast ratchets are not sample accurate.
- A full event queue drops additional glitch events, preserving audio real-time performance.
- Performer master applies to Drum 2–7 only, rather than offering a configurable per-voice inclusion mask.

## Testing status

GitHub Actions runs host regression tests and compiles the ARM firmware, including memory reporting. Real hardware validation is still required for trigger jitter, voice choke behavior, menu navigation, patch/kit compatibility and CPU/audio load. Start with all Mode knobs at 0, use moderate monitor volume and keep the working V4 image for restoration.

**Do not describe this as exact IDUM or hardware-validated firmware.**
