# HOME Menu banner, icon and sound variants

The other versions of the HOME Menu assets, kept in case one of them replaces the one in use. They were made by the
owner (images and the "Beyond The Final Gate" track generated with AI, Gemini for the music) to match the Zero
Mission port's. None of this folder goes into the build: only the files in `resources/` do (see the Makefile).

## In use (`resources/`)

- `banner.png`: Samus on guard (256x128 RGBA), the same as `banner_samus_on_guard.png` here.
- `icon.png`: baby Metroid (48x48 RGBA), the same as `icon_baby_metroid.png` here.
- `audio/audio.wav`: "Beyond The Final Gate", a 2.93 s cut (44.1 kHz, 16-bit stereo PCM); its source is
  `audio/Beyond_The_Final_Gate_trimmed_2.9s.mp3`.

Two things seen on the console (also noted by the Makefile): the banner sound must stay under about 3 s (the 3.5 s
cut did not play), and the HOME Menu only shows a new icon, banner or sound once the installed title's version goes
up, so try a change with a CIA built as the next version (`make VERSION=vX.Y.Z+1-test cia`).

## Variants

Banners (256x128 RGBA):
- `banner_samus_on_guard.png`: Samus in the Varia suit on guard, facing out (in use).
- `banner_samus_and_metroid.png`: Samus kneeling, arm cannon up, the baby Metroid beside her.
- `banner_original_snes.png`: the earlier banner (the SNES helmet on black).

Icons (48x48 RGBA):
- `icon_baby_metroid.png`: the baby Metroid in a green energy ring (in use).
- `icon_screwattack_green.png`: the Screw Attack emblem in a green plasma ring.
- `icon_original_chozo.png`: the earlier icon (the Chozo statue on an orange disc).

Sound (`audio/`):
- `Beyond_The_Final_Gate_trimmed_2.9s.mp3`: the source of the cut in use.
- `Beyond_The_Final_Gate_trimmed_3.5s.mp3`: an earlier cut, too long to play on the HOME Menu.
- `Beyond_The_Final_Gate_full.mp3`: the whole track.
- `audio_original_chimes.wav`: the earlier banner sound (tubular bells with the Metroid motif).
- `audio_long_original.wav`: a longer version of it.

Sources (`highres/`), the full-size pictures the banners and icons were made from, and `comparison_showcase.png`,
every variant beside the Zero Mission port's.

## Switching

Copy the variant over the file in use, then build (as the next version to see it on the console):

```sh
cp resources/variants/banner_samus_and_metroid.png resources/banner.png
cp resources/variants/icon_screwattack_green.png resources/icon.png
```

A sound has to be a WAV for bannertool: convert the MP3 (for example
`ffmpeg -i in.mp3 -ar 44100 -ac 2 -c:a pcm_s16le resources/audio/audio.wav`) and keep it under 3 s.
