# Sample-mode development — not flashable

Requested interface: per-drum Oscillator page switch `SYN` / `SMP` and a sample selector. SYN uses existing synthesis unchanged; SMP plays a one-shot from a small custom bank instead of that voice's synth engine. The working Rumble and IDM baseline is `fix/idm-event-playback-v1561`.

## Findings from actual source

- The existing transient bank contains **12** short 8-bit signed mono samples, each **2205 samples (~50 ms at 44.1 kHz)**; it is compiled into `transientTables.c` and stored using `INDTCM` (fast RAM).
- Factory click samples do not contain long clap/open-hat tails. Replacing the bytes in-place would cut each imported sample to ~50 ms; not appropriate for full drum one-shots.
- Drum voices share different synth engines. The LXR exposes seven note targets (including separate open and closed hat triggers), but the hat voices share the same audio engine and choke behavior.
- A custom bank should therefore live in **flash** or another validated memory region, with limited mono samples at a known fixed rate, not in the existing tightly limited 128 KB DTCM.
- SD card currently stores kits and firmware updates; arbitrary real-time WAV playback is not already implemented. An SD sample loader needs independent engineering and latency tests.

## Current commit

`SampleVoiceBank.c/.h` is a standalone *prototype* supporting per-voice sample enabling, sample index and trigger/retrigger. It uses the stock 50ms transient slots for a basic signal path proof of concept. **Not wired to the oscillator menu, mixer, kit storage, or sample imports.** Do not flash this branch expecting sample mode.

## Next engineering milestones

1. Decide maximum custom sample length and count from the supplied WAVs; use firmware-embedded samples initially for reliable timing.
2. Convert only the user's files into a flash-resident sample bank (mono PCM with explicit lengths), without deleting original transient samples in the baseline.
3. Switch between synth and sample rendering at each voice, preserve original output routing, volume and mono kick rumble.
4. Preserve closed-hat choke of open-hat sample playback; ensure samples retrigger precisely from start.
5. Add SYN/SMP selection and sample slot to each oscillator page, with versioned per-kit save/restore and factory default SYN.
6. Host tests: silence when inactive, audio length, retrigger, voice isolation and choke. ARM build and memory audit.
7. Test playback timing/audio on hardware before releasing a firmware image.

Source licensed GPL-3.0-only; modified firmware must be labeled unofficial.
