"""Generate seamless office room textures for RulesHorror (Python 3 + numpy + Pillow).

Run:  python create_office_room_textures.py
Output (1024 x 1024 PNG, all tile in both directions), next to this script:
    T_OfficeWall_BaseColor / _Normal      painted plaster wall, one tile = 2 m
    T_OfficeCarpet_BaseColor / _Normal    50 cm carpet tiles (2 x 2 per texture), one tile = 1 m
    T_OfficeCeiling_BaseColor / _Normal   60 cm acoustic tiles in a T-bar grid (2 x 2 per texture), one tile = 1.2 m

Normal maps are DirectX style (Unreal): green = +V (down the image).
Tile seams in the carpet and ceiling textures sit at 0 and 0.5 along U and at 0.25 and 0.75 along V, so
with the room mesh UVs (1 UV unit = 1 m, origin at the room center) the border tiles come out symmetric.
Replaces the engine default checker on the L_Office BSP room.
"""

from pathlib import Path

import numpy as np
from PIL import Image

SIZE = 1024
OUT_DIR = Path(__file__).resolve().parent


# -----------------------------------------------------------------------------
# Helpers
# -----------------------------------------------------------------------------

def periodic_noise(rng, size, scale_x, scale_y, power=2.0):
    """Tileable noise (random spectrum with a falloff, inverse FFT). Bigger scale = smoother along that axis."""
    fx = np.fft.fftfreq(size)[None, :] * scale_x
    fy = np.fft.fftfreq(size)[:, None] * scale_y
    f = np.sqrt(fx * fx + fy * fy)
    f[0, 0] = 1.0
    spectrum = (rng.normal(size=(size, size)) + 1j * rng.normal(size=(size, size))) / f ** power
    spectrum[0, 0] = 0.0
    n = np.real(np.fft.ifft2(spectrum))
    return (n - n.mean()) / (n.std() + 1e-8)


def band_noise(rng, size, lo, hi):
    """Tileable noise limited to a frequency band (cycles per texture), roughly isotropic."""
    fx = np.fft.fftfreq(size)[None, :] * size
    fy = np.fft.fftfreq(size)[:, None] * size
    f = np.sqrt(fx * fx + fy * fy)
    mask = ((f >= lo) & (f <= hi)).astype(np.float64)
    spectrum = (rng.normal(size=(size, size)) + 1j * rng.normal(size=(size, size))) * mask
    n = np.real(np.fft.ifft2(spectrum))
    return (n - n.mean()) / (n.std() + 1e-8)


def tile_coords(size, tiles, v_offset):
    """Per-pixel tile index and local 0..1 coordinates for a tiles x tiles grid (V shifted by v_offset)."""
    u = (np.arange(size, dtype=np.float64)[None, :] + 0.5) / size
    v = (np.arange(size, dtype=np.float64)[:, None] + 0.5) / size
    v = (v - v_offset) % 1.0
    tu, tv = u * tiles, v * tiles
    iu, iv = np.floor(tu).astype(int), np.floor(tv).astype(int)
    u = np.broadcast_to(tu - iu, (size, size))
    v = np.broadcast_to(tv - iv, (size, size))
    iu = np.broadcast_to(iu, (size, size)) % tiles
    iv = np.broadcast_to(iv, (size, size)) % tiles
    return iu, iv, u, v


def normal_from_height(height, strength):
    """DirectX tangent-space normal map from a tileable height field (in pixels of height per pixel)."""
    du = (np.roll(height, -1, axis=1) - np.roll(height, 1, axis=1)) * 0.5
    dv = (np.roll(height, -1, axis=0) - np.roll(height, 1, axis=0)) * 0.5
    nx, ny, nz = -du * strength, -dv * strength, np.ones_like(height)
    length = np.sqrt(nx * nx + ny * ny + nz * nz)
    n = np.stack([nx, ny, nz], axis=-1) / length[..., None]
    return np.clip((n * 0.5 + 0.5) * 255.0 + 0.5, 0, 255).astype(np.uint8)


def to_rgb8(color):
    return np.clip(color + 0.5, 0, 255).astype(np.uint8)


def save(array, name):
    path = OUT_DIR / name
    Image.fromarray(array, "RGB").save(path)
    print(f"Wrote {path}")


def blur(field, radius):
    """Tileable box-ish blur via FFT (gaussian)."""
    fx = np.fft.fftfreq(field.shape[1])[None, :]
    fy = np.fft.fftfreq(field.shape[0])[:, None]
    g = np.exp(-2.0 * (np.pi ** 2) * (radius ** 2) * (fx * fx + fy * fy))
    return np.real(np.fft.ifft2(np.fft.fft2(field) * g))


# -----------------------------------------------------------------------------
# Painted plaster wall (one tile = 2 m)
# -----------------------------------------------------------------------------

def make_wall(rng):
    paint = np.array([198.0, 193.0, 178.0])     # warm off-white (sRGB)
    blotch = periodic_noise(rng, SIZE, 1.0, 1.0, 2.4)       # large, soft tone changes
    mottle = band_noise(rng, SIZE, 20, 60)                   # roller mottling
    peel = band_noise(rng, SIZE, 180, 400)                   # orange-peel texture
    specks = (rng.random((SIZE, SIZE)) > 0.9993).astype(np.float64)
    specks = blur(specks, 0.8) * 6.0

    tone = 1.0 + 0.015 * blotch + 0.012 * mottle + 0.006 * peel - 0.10 * np.clip(specks, 0, 1)
    base = paint[None, None, :] * tone[..., None]
    save(to_rgb8(base), "T_OfficeWall_BaseColor.png")

    height = 0.6 * peel + 0.25 * mottle
    save(normal_from_height(height, 0.35), "T_OfficeWall_Normal.png")


# -----------------------------------------------------------------------------
# Carpet tiles (50 cm, 2 x 2 per texture, quarter-turn laid)
# -----------------------------------------------------------------------------

def make_carpet(rng):
    tiles = 2
    iu, iv, u, v = tile_coords(SIZE, tiles, 0.25)
    carpet = np.array([74.0, 80.0, 90.0])       # blue-gray (sRGB)

    # Fibre structure: fine noise plus a slight pile direction that turns 90 degrees tile by tile.
    fine = band_noise(rng, SIZE, 250, 500)
    tufts = band_noise(rng, SIZE, 90, 200)
    streak_u = periodic_noise(rng, SIZE, 30.0, 2.0, 1.2)
    streak_v = periodic_noise(rng, SIZE, 2.0, 30.0, 1.2)
    quarter = (iu + iv) % 2 == 0
    streak = np.where(quarter, streak_u, streak_v)
    flecks = band_noise(rng, SIZE, 120, 260)
    fleck_mask = np.clip((flecks - 2.2) * 2.0, 0, 1)        # sparse lighter and darker yarn flecks
    fleck_dark = np.clip((-flecks - 2.4) * 2.0, 0, 1)

    # Per-tile shade (dye lot / pile direction) and traffic wear.
    tile_shade = rng.normal(0.0, 0.025, size=(tiles, tiles))[iv, iu]
    wear = periodic_noise(rng, SIZE, 1.0, 1.0, 2.6)

    # Seams: thin dark line where tiles meet.
    edge = np.minimum(np.minimum(u, 1 - u), np.minimum(v, 1 - v)) * (SIZE / tiles)   # pixels to the nearest seam
    seam = np.exp(-(edge ** 2) / 2.0)

    tone = (1.0 + 0.05 * fine + 0.04 * tufts + 0.012 * streak + tile_shade + 0.03 * wear
            + 0.18 * fleck_mask - 0.18 * fleck_dark - 0.35 * seam)
    base = carpet[None, None, :] * tone[..., None]
    save(to_rgb8(base), "T_OfficeCarpet_BaseColor.png")

    height = 0.8 * fine + 0.7 * tufts - 3.0 * seam
    save(normal_from_height(height, 0.4), "T_OfficeCarpet_Normal.png")


# -----------------------------------------------------------------------------
# Acoustic ceiling tiles (60 cm) in a white T-bar grid, 2 x 2 per texture
# -----------------------------------------------------------------------------

def make_ceiling(rng):
    tiles = 2
    iu, iv, u, v = tile_coords(SIZE, tiles, 0.25)
    px = SIZE / tiles                              # pixels per 60 cm tile
    tile_col = np.array([214.0, 210.0, 199.0])
    grid_col = np.array([228.0, 227.0, 222.0])

    edge = np.minimum(np.minimum(u, 1 - u), np.minimum(v, 1 - v)) * px   # pixels to the grid line center
    grid_half = px * (1.2 / 60.0)                  # 2.4 cm wide T-bar face
    bevel = px * (0.8 / 60.0)                      # tile edge rolls down into the grid

    # Fissured mineral fibre: worm-like dark pits from thresholded band noise, plus pinholes.
    fiss_a = band_noise(rng, SIZE, 60, 140)
    fiss_b = band_noise(rng, SIZE, 60, 140)
    fissure = np.clip(1.0 - np.abs(fiss_a) * 3.2, 0, 1) * np.clip(fiss_b * 1.5 + 0.3, 0, 1)
    pins = (rng.random((SIZE, SIZE)) > 0.997).astype(np.float64)
    pins = np.clip(blur(pins, 0.7) * 5.0, 0, 1)
    grain = band_noise(rng, SIZE, 200, 450)
    stain = np.clip(periodic_noise(rng, SIZE, 1.0, 1.0, 2.8) - 1.2, 0, None)   # rare water stains
    tile_shade = rng.normal(0.0, 0.015, size=(tiles, tiles))[iv, iu]

    grid = (edge < grid_half).astype(np.float64)
    roll = np.clip((edge - grid_half) / bevel, 0, 1)        # 0 at grid, 1 on the flat tile face
    tile_tone = (1.0 + 0.02 * grain + tile_shade - 0.22 * fissure - 0.30 * pins - 0.05 * stain
                 - 0.18 * (1 - roll))
    tile = tile_col[None, None, :] * tile_tone[..., None]
    stain_tint = np.array([0.0, -6.0, -18.0])[None, None, :] * np.clip(stain, 0, 1)[..., None]
    tile = tile + stain_tint
    grid_shade = grid_col[None, None, :] * (1.0 + 0.01 * grain[..., None])
    base = np.where(grid[..., None] > 0.5, grid_shade, tile)
    save(to_rgb8(base), "T_OfficeCeiling_BaseColor.png")

    # Height: grid face proud, tile face recessed with a rolled edge, fissures and pins dug in.
    height = np.where(grid > 0.5, 3.0, 3.0 * (1 - roll) * 0.6) - 1.2 * fissure - 1.5 * pins + 0.2 * grain
    save(normal_from_height(height, 0.6), "T_OfficeCeiling_Normal.png")


def main():
    make_wall(np.random.default_rng(11))
    make_carpet(np.random.default_rng(23))
    make_ceiling(np.random.default_rng(37))


if __name__ == "__main__":
    main()
