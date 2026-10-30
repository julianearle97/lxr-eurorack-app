# Custom Drum 1 mono rumble (experimental)

**Unofficial modified firmware. Not tested on physical hardware.**
Retains the factory firmware on `main`, and keeps all development on `feature/drum1-mono-rumble`.

## User controls

On Drum 1, select **Mix** and use the encoder to scroll past the factory Mix controls into the new **Rumble** group. The four panel knobs now map to:

1. **Lvl** — wet rumble mix, 0 = original dry kick (factory default).
2. **Dcy** — rumble feedback/decay.
3. **LPF** — rumble low-pass cutoff, logarithmic ~60–1000 Hz.
4. **Dck** — transient-triggered wet-only ducking.

A fixed **25 Hz high-pass** removes excessive subsonic rumble without requiring a knob. The effect is **mono** and is inserted after Drum 1 voice processing but before the original output routing. It follows Drum 1's output assignment; the global FX bus remains separate.

## Kit compatibility

An 8-byte `RMB\x01` extension is appended to new `.snd` drum-kit files, following the original factory sound data. On loading old kit files without this extension the effect defaults to dry; the original sound parameter indices remain unchanged. **Back up your SD card before any experimental firmware tests.**

## Verification status

- Host DSP tests: automated in GitHub Actions.
- ARM firmware compilation: automated in GitHub Actions.
- Real-time processor load and acoustic quality: **not verified on physical hardware**.
- Kit saving, loading and menu navigation: **compiled but not tested on physical hardware**.
- No oscilloscope/audio loopback hardware tests available here.

## Important first-flash precautions

Keep a copy of official firmware and a full SD-card backup. Bootloader updates are **not** required. Use only the standard updater procedure in the repository README. Do not run `flash_full` (this would overwrite the bootloader).

Start with Rumble Level = 0 and headphones/monitor volume low; test one low-volume kick first, then gradually enable rumble. Monitor for clipping, unexpected audio or timing glitches. If anything behaves incorrectly, return to factory firmware using the SD updater.

## Future work

- Hardware listening and DSP profiling with six simultaneous voices and global FX.
- Better bypass smoothing and parameter response.
- Add click-free controls, verify repeated/retriggered kick timing.
- IDUM-style glitch engine remains **out of scope** for this branch.
