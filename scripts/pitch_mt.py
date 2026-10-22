"""Pitch the Detroit low tom up toward a 909 mid tom and write samples/mt.wav."""
import array
import wave
from pathlib import Path

src = Path("samples/lt.wav")
dst = Path("samples/mt.wav")
ratio = 122.0 / 96.0

with wave.open(str(src), "rb") as w:
    channels = w.getnchannels()
    width = w.getsampwidth()
    rate = w.getframerate()
    frames = w.readframes(w.getnframes())

if width != 2 or channels != 1:
    raise SystemExit(f"unexpected wav format ch={channels} width={width}")

data = array.array("h")
data.frombytes(frames)
out_len = max(1, int(len(data) / ratio))
out = array.array("h")
for i in range(out_len):
    pos = i * ratio
    i0 = int(pos)
    i1 = min(i0 + 1, len(data) - 1)
    frac = pos - i0
    sample = data[i0] * (1.0 - frac) + data[i1] * frac
    out.append(max(-32768, min(32767, int(sample))))

with wave.open(str(dst), "wb") as w:
    w.setnchannels(1)
    w.setsampwidth(2)
    w.setframerate(rate)
    w.writeframes(out.tobytes())

print("mt", dst.stat().st_size)
