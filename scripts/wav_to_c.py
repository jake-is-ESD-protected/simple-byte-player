#!/usr/bin/env python3
"""Convert a 16-bit PCM WAV file into the player's C audio array."""

import argparse
import pathlib
import struct
import wave


def convert(input_path: pathlib.Path, output_path: pathlib.Path) -> None:
    with wave.open(str(input_path), "rb") as wav:
        channels = wav.getnchannels()
        sample_width = wav.getsampwidth()
        sample_rate = wav.getframerate()
        frames = wav.readframes(wav.getnframes())

    if sample_width != 2:
        raise ValueError("only 16-bit PCM WAV files are supported")
    if channels not in (1, 2):
        raise ValueError("only mono and stereo WAV files are supported")

    values = struct.unpack("<" + "h" * (len(frames) // 2), frames)
    if channels == 2:
        # MAX98357A is mono. Mix stereo input to mono before storing it.
        values = tuple(
            (left + right) // 2
            for left, right in zip(values[::2], values[1::2])
        )

    with output_path.open("w", encoding="utf-8") as output:
        output.write("#pragma once\n\n#include <Arduino.h>\n\n")
        output.write(f"#define AUDIO_SAMPLE_RATE {sample_rate}u\n")
        output.write(f"#define AUDIO_NUM_SAMPLES {len(values)}u\n\n")
        output.write("static const int16_t audio_data[AUDIO_NUM_SAMPLES] = {\n")
        for offset in range(0, len(values), 12):
            chunk = values[offset : offset + 12]
            output.write("    " + ", ".join(str(value) for value in chunk) + ",\n")
        output.write("};\n")


def run_from_platformio() -> None:
    # PlatformIO executes this file through `extra_scripts = pre:...`.
    project_dir = pathlib.Path(env.subst("$PROJECT_DIR"))
    config = env.GetProjectConfig()
    section = "env:" + env["PIOENV"]
    input_name = config.get(section, "audio_wav", "audio/test_sine.wav")
    output_name = config.get(section, "audio_header", "src/audio_data.h")
    input_path = project_dir / input_name
    output_path = project_dir / output_name
    convert(input_path, output_path)
    print(f"audio: {input_path} -> {output_path}")


def run_from_command_line() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("input", type=pathlib.Path, help="16-bit PCM WAV input")
    parser.add_argument(
        "output",
        type=pathlib.Path,
        nargs="?",
        default=pathlib.Path("src/audio_data.h"),
        help="output C header (default: src/audio_data.h)",
    )
    args = parser.parse_args()
    convert(args.input, args.output)
    print(f"wrote {args.output}")


try:
    Import("env")
except NameError:
    env = None

if env is not None:
    run_from_platformio()
elif __name__ == "__main__":
    run_from_command_line()
