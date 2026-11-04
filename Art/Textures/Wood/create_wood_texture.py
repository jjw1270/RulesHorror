"""Generate a seamless desk-wood base color texture for RulesHorror (Python 3 + numpy + Pillow).

Run:  python create_wood_texture.py
Output: T_Wood_Desk_BaseColor.png (1024 x 1024, tiles in both directions, grain runs along U).

Replaces the purchased BrutalistLevelKit wood (MI_Desk_Wood / T_GenericWood_BC) on the L_Lobby desk top
and the office desks. One tile is meant to cover about 1 m.
"""

from pathlib import Path

import numpy as np
from PIL import Image

SIZE = 1024
SEED = 7
OUT = Path(__file__).resolve().parent / "T_Wood_Desk_BaseColor.png"

# sRGB colors (0-255), dark grain to light wood.
DARK = np.array([92, 66, 48], dtype=np.float32)
MID = np.array([128, 94, 70], dtype=np.float32)
LIGHT = np.array([150, 114, 86], dtype=np.float32)


def periodic_noise(rng, size, scale_x, scale_y, power=2.0):
    """Tileable noise: random spectrum shaped by a falloff, back to image space with an inverse FFT.
    scale_x/scale_y damp frequencies along that axis (bigger = longer, smoother features along it)."""
    fx = np.fft.fftfreq(size)[None, :] * scale_x
    fy = np.fft.fftfreq(size)[:, None] * scale_y
    f = np.sqrt(fx * fx + fy * fy)
    f[0, 0] = 1.0
    spectrum = (rng.normal(size=(size, size)) + 1j * rng.normal(size=(size, size))) / f ** power
    spectrum[0, 0] = 0.0
    n = np.real(np.fft.ifft2(spectrum))
    return (n - n.mean()) / (n.std() + 1e-8)


def main():
    rng = np.random.default_rng(SEED)
    y = np.arange(SIZE, dtype=np.float32)[:, None] / SIZE  # grain runs along x (U)

    warp = periodic_noise(rng, SIZE, 4.0, 1.0, 2.0)            # wavy grain, long along U
    detail = periodic_noise(rng, SIZE, 8.0, 1.0, 1.6)          # small kinks in the grain lines
    field = y * 14.0 + warp * 1.4 + detail * 0.25
    phase = field - np.floor(field)
    rings = np.exp(-((phase - 0.5) ** 2) / 0.006)               # thin dark late-wood lines
    rings_soft = np.exp(-((phase - 0.5) ** 2) / 0.05)           # darker band around each line
    streaks = periodic_noise(rng, SIZE, 14.0, 0.6, 1.4)         # long streaks along the grain
    fibers = periodic_noise(rng, SIZE, 40.0, 0.4, 1.0)          # fine fibers along the grain
    blotch = periodic_noise(rng, SIZE, 2.0, 2.0, 2.6)           # gentle large tone variation

    t = 0.58 + 0.13 * streaks + 0.10 * fibers + 0.08 * blotch - 0.08 * rings_soft
    t = np.clip(t, 0.0, 1.0)
    base = np.where(t[..., None] < 0.5,
                    DARK + (MID - DARK) * (t[..., None] / 0.5),
                    MID + (LIGHT - MID) * ((t[..., None] - 0.5) / 0.5))
    base = base * (1.0 - 0.16 * rings[..., None])               # darken along grain lines
    img = np.clip(base, 0, 255).astype(np.uint8)
    Image.fromarray(img, "RGB").save(OUT)
    print(f"Wrote {OUT}")


if __name__ == "__main__":
    main()
