# LXR Sample Mode — finishing the custom audio bank

The firmware source now has the selectable four-knob OSC sample page (SYN/SMP / Slot / Pitch / Volume), seven-voice sample trigger routing, mixer replacement, Drum 1 rumble compatibility, basic hi-hat choking and optional per-kit SMP settings.

**IMPORTANT: Do not flash a firmware image from this branch until the supplied custom sample data has replaced the empty placeholder.** CI deliberately rejects the placeholder.

## Required asset upload
The user-provided WAV files were converted into 407,759 bytes of signed 8-bit mono PCM at 44.1 kHz with all original lengths preserved. Because the large generated binary-to-C file is not yet in GitHub, **replace**:
`Core/Src/Soundengine/UserSampleBank.c`
with the `UserSampleBank.c` file supplied in the chat (about 1 MB of source / 398 KiB flash).

From GitHub branch `feature/sample-voice-mode-user-wavs`, open `Core/Src/Soundengine/UserSampleBank.c` and use the GitHub UI to replace its contents or delete placeholder and upload the complete file. Commit to the *same branch*, not `main`.

After the upload, the automated build should run host DSP regression tests, compile the ARM image and check its memory use.

## Audio behavior
- Modes default to SYN for factory kit compatibility.
- SMP renders a retriggerable full-length one-shot into the normal voice output path, before Drum 1's mono rumble.
- The 12-sample bank is read from const FLASH, not 128-KiB DTCM.
- Slot labels 1–12, center Pitch 64, Volume 0–127.
- Closed-hat sample playback stops open-hat sample playback.
- Existing LXR IDM glitch events feed the same note-on dispatch path.

## Important hardware-test caveats
This is a new audio path; compile success alone cannot verify audible playback, stereo-downmix quality, encoder page navigation, CPU load, per-kit recall or mixed SYN/SMP closed/open hat behavior. Test at low output volume and keep the previous working v1.56.1 firmware on the SD-card backup.

The code remains unofficial custom firmware, not an Erica Synths release.
