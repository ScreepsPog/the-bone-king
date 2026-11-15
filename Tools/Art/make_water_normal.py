"""Plain Python (no Unreal, no other packages): writes a normal map of rippled water that repeats without a seam, for
the river's material. The project has no water texture and the engine ships none that fit, so this one is made here.

The surface is a sum of waves whose counts across the picture are whole numbers (that is what makes it repeat):
a few long swells and many small ripples, some of them sharpened into crests. The normal at each point is worked
out from the slope of that surface.

Run: py Tools/Art/make_water_normal.py   (writes SourceArt/Level/T_Water_Normal.png, 512 by 512)"""
import math
import os
import random
import struct
import zlib

SIZE = 512
STRENGTH = 5.0
OUT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "SourceArt", "Level", "T_Water_Normal.png")

random.seed(7)
waves = []
for count in range(26):
    # Whole numbers of waves each way; more small ones than large ones.
    reach = random.choice([1, 2, 2, 3, 3, 4, 5, 6, 7, 9, 11, 13])
    angle = random.uniform(0.0, math.pi)
    kx = round(math.cos(angle) * reach)
    ky = round(math.sin(angle) * reach) or 1
    size = math.hypot(kx, ky)
    waves.append((kx, ky, random.uniform(0.0, math.tau), 1.0 / size ** 1.15, random.random() < 0.45))

TABLE = [math.sin(math.tau * i / 4096.0) for i in range(4096)]


def sine(turns):
    return TABLE[int(turns * 4096.0) & 4095]


rows = []
for y in range(SIZE):
    row = bytearray([0])
    v = y / SIZE
    for x in range(SIZE):
        u = x / SIZE
        slope_u = 0.0
        slope_v = 0.0
        for kx, ky, phase, size, crest in waves:
            turns = kx * u + ky * v + phase / math.tau
            # The slope of sin is cos; of a sharpened crest (1 - |sin|), minus the sign of sin times cos.
            cos = sine(turns + 0.25)
            slope = cos if not crest else (-cos if sine(turns) > 0.0 else cos) * 1.4
            slope_u += slope * kx * size
            slope_v += slope * ky * size
        nx = -slope_u * STRENGTH / 26.0
        ny = -slope_v * STRENGTH / 26.0
        length = math.sqrt(nx * nx + ny * ny + 1.0)
        row += bytes((int((nx / length * 0.5 + 0.5) * 255.0 + 0.5), int((ny / length * 0.5 + 0.5) * 255.0 + 0.5), int((1.0 / length * 0.5 + 0.5) * 255.0 + 0.5)))
    rows.append(bytes(row))


def chunk(kind, data):
    body = kind + data
    return struct.pack(">I", len(data)) + body + struct.pack(">I", zlib.crc32(body) & 0xFFFFFFFF)


os.makedirs(os.path.dirname(OUT), exist_ok=True)
with open(OUT, "wb") as file:
    file.write(b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", struct.pack(">IIBBBBB", SIZE, SIZE, 8, 2, 0, 0, 0)) + chunk(b"IDAT", zlib.compress(b"".join(rows), 9)) + chunk(b"IEND", b""))
print("wrote", os.path.abspath(OUT), SIZE)
