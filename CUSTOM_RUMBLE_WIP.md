# Drum 1 mono rumble — work in progress

This branch is an **unflashed experiment**, not a firmware release.

## Intended behavior

- A strictly mono kick-generated reverb/rumble, mixed with Drum 1 **before** its existing output assignment.
- Drum 1 Mix page: Level, Decay, HP, LP and Duck. All other drum pages unchanged.
- Kick trigger restarts ducking but does not clear the reverb tail.
- Level = 0 yields unaltered dry kick.
- No use of the global FX bus; global effects stay available.
- Stored per-kit, with older kits defaulting to rumble off.

## Implemented here

- Standalone `KickRumble.h` / `KickRumble.c` prototype: mono delay network, diffuser, high-pass, low-pass and trigger-initiated wet ducking.
- This code is **not yet linked into CMake, menu, mixer, trigger engine, or presets**, deliberately avoiding a possibly unsafe hardware image.

## Required integration before a hardware build

1. Determine actual engine sample rate and memory sections; current prototype consumes ~44 KB of float delay memory and has fixed delay sizes designed for roughly 44.1 kHz.
2. Confirm processor budget and saturation behavior with all six voices + normal FX.
3. Allocate from an appropriate SRAM section; do not place the buffer in the limited 128 KB DTCM inadvertently.
4. Call `KickRumble_trigger` at the Drum 1 event, not from raw input CV.
5. Process Drum 1's mono voice buffer **before** `mixer_addDataToOutput*` so the wet signal follows the existing routing.
6. Add per-voice Mix UI values and safe bounds; persist them via versioned/compatible kit format.
7. Keep a full bypass at level 0 and verify no stale audible tail when turning the effect back on.
8. Verify the source builds with the documented ARM toolchain and check SRAM/ITCM use.
9. Listen for glitches, gain change, unstable feedback, excessive sub frequencies, and ducking clicks before distributing any update image.

**Do not flash a binary based on this prototype alone.**
