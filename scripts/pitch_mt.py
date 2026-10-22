"""Trim the downloaded Detroit kit to 16-bit mono and pitch a mid tom."""
import array
import struct
import wave
from pathlib import Path

ROOT = Path("samples")
NAMES = ["bd", "sd", "lt", "ht", "rs", "cp", "ch", "oh", "cr", "rd"]

def read_wav(path):
    data = path.read_bytes()
    if data[:4] != b"RIFF":
        raise SystemExit(f"not a wav: {path}")
    pos = 12
    fmt = None
    pcm = None
    while pos + 8 <= len(data):
        cid = data[pos:pos + 4]
        size = struct.unpack_from("<I", data, pos + 4)[0]
        chunk = data[pos + 8:pos + 8 + size]
        if cid == b"fmt ":
            fmt = chunk
        elif cid == b"data":
            pcm = chunk
            break
        pos += 8 + size + (size & 1)
    if fmt is None or pcm is None:
        raise SystemExit(f"incomplete wav: {path}")
    audio_format, channels, rate, _byte_rate, _align, bits = struct.unpack_from("<HHIIHH", fmt)
    if audio_format != 1 or bits not in (16, 24) or channels < 1:
        raise SystemExit(f"unsupported {path} format={audio_format} bits={bits} ch={channels}")
    width = bits // 8
    frames = len(pcm) // (width * channels)
    samples = []
    for i in range(frames):
        total = 0.0
        for c in range(channels):
            offset = (i * channels + c) * width
            if bits == 16:
                value = struct.unpack_from("<h", pcm, offset)[0] / 32768.0
            else:
                b0, b1, b2 = pcm[offset:offset + 3]
                value = b0 | (b1 << 8) | (b2 << 16)
                if value & 0x800000:
                    value -= 0x1000000
                value = value / 8388608.0
            total += value
        samples.append(total / channels)
    return rate, samples

def trim(samples, rate):
    threshold = 10 ** (-46 / 20)
    first = next((i for i, sample in enumerate(samples) if abs(sample) > threshold), 0)
    last = next((i for i in range(len(samples) - 1, -1, -1) if abs(samples[i]) > threshold), 0)
    start = max(0, first - int(0.001 * rate))
    end = min(len(samples), last + int(0.015 * rate) + 1)
    if end <= start:
        end = min(len(samples), start + 1)
    return samples[start:end]

def write_wav(path, rate, samples):
    out = array.array("h")
    for sample in samples:
        out.append(int(max(-1.0, min(1.0, sample)) * 32767))
    with wave.open(str(path), "wb") as handle:
        handle.setnchannels(1)
        handle.setsampwidth(2)
        handle.setframerate(rate)
        handle.writeframes(out.tobytes())

decoded = {}
for name in NAMES:
    rate, samples = read_wav(ROOT / f"{name}.wav")
    decoded[name] = (rate, trim(samples, rate))

rate, low_tom = decoded["lt"]
ratio = 122.0 / 96.0
mid = []
for i in range(max(1, int(len(low_tom) / ratio))):
    pos = i * ratio
    i0 = int(pos)
    i1 = min(i0 + 1, len(low_tom) - 1)
    frac = pos - i0
    mid.append(low_tom[i0] * (1.0 - frac) + low_tom[i1] * frac)
decoded["mt"] = (rate, mid)

for name, (rate, samples) in decoded.items():
    write_wav(ROOT / f"{name}.wav", rate, samples)
    print(name, len(samples))
