# LXR sample mode: uploaded WAVs and pitch/volume prototype

## User sample bank

12 WAVs supplied, all 44.1 kHz, stereo 16-bit PCM. Exact total duration 9.246 s. Converted to **407,759 bytes** signed mono 8-bit 44.1 kHz PCM with filenames, offsets and lengths preserved in a separate manifest. The assets are provided as `lxr_sample_bank_assets.zip` in the ChatGPT conversation. These bytes are **not yet embedded in firmware**.

User-requested four-knob oscillator sample page:

1. SYN/SMP (factory default SYN)
2. Sample slot (1–12)
3. Pitch (center 64, roughly +/-24 semitones)
4. Volume (0–127)

**SYN must retain existing oscillator functionality**; when SMP is enabled, audio comes from a selected full-length one-shot. Existing voice volume, output assignment, mute, MIDI triggering, custom rumble and IDM routing must remain intact.

## Code completed so far

`Core/Src/Soundengine/SampleVoiceBank.c/.h` implement an **isolated prototype**, with full-length PCM8 slots, 64-bit fractional phase, pitch-rate control, per-voice volume and velocity, retrigger and explicit stop (needed for open-hat choking). Host regression tests are included in CI.

**Not flash-ready.** The uploaded sample bank isn't in GitHub, the DSP prototype isn't in the ARM link, and the oscillator menu, per-kit save, per-voice routing and shared hi-hat handling have not been integrated.

The stock 12 click transients are 50 ms each and are held in DTCM, so they cannot simply hold these longer files in their existing allocation. For 9.246 s at 44.1 kHz, our compact PCM8 bank needs about 398 KiB of additional flash; final image and CPU usage require measurement.

## Remaining work

- Transfer the PCM8 bank and manifest into the project build, as read-only flash data with lengths.
- Add the four-knob SYN/SMP/Slot/Pitch/Volume pages while preserving regular Osc page behavior.
- Bypass synth engine only for voices in SMP mode; route samples through normal mixer.
- Implement closed-hat choke of the open-hat sample; independent timing and note input behavior.
- Versioned kit persistence, old kits default SYN.
- Build/size/test with all existing rumble+IDM features. Hardware first-play testing with backup firmware.
