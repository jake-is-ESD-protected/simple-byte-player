# simple-byte-player

Simple ESP32 I2S output loop from audio stored in flash memory.  It targets an
AZ-Delivery ESP32 Dev Kit and a MAX98357A mono I2S amplifier.

## Wiring

| MAX98357A | ESP32 |
|---|---:|
| DIN | GPIO12 |
| BCLK | GPIO14 |
| LRC / WS | GPIO25 |
| GND | GND |
| VIN | suitable amplifier supply |

The player sends the mono sample to both I2S slots, so the amplifier's left /
right selection does not matter.

## Build and upload

```sh
pio run -t upload
```

Control playback through jescore:

```sh
jescore audio on
jescore audio off
jescore audio state
jescore stats
```

`src/audio_data.h` currently contains a one-second, 440 Hz, 16-bit test sine.
The player repeats it indefinitely.

## Convert a WAV file

The converter accepts mono or stereo, 16-bit PCM WAV files. Stereo input is
mixed to mono because the MAX98357A is a mono amplifier.

The conversion runs automatically as a PlatformIO pre-script:

```sh
pio run -t upload
```

By default it converts `audio/test_sine.wav`. To select another file, set
these options in `platformio.ini`:

```ini
audio_wav = audio/input.wav
audio_header = src/audio_data.h
```

The script can also be run directly:

```sh
python3 scripts/wav_to_c.py input.wav src/audio_data.h
```

The generated header is intentionally kept separate from `src/main.cpp`, so
replacing audio does not change the player implementation.
