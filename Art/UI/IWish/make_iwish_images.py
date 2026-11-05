"""Images for the in-game i-wish homepage: 2004 low-res digital camera 'ghost photos', the title banner photo and
rough ink stamp textures. Run after make_iwish_photos.py (Blender renders in Renders/):
    python make_iwish_images.py [--renders <dir>]
Writes T_IW_*.png next to this file (import to /Game/UI/Office/Monitor/Textures/IWish, default texture filter).
Pixel text uses Galmuri11 (SIL OFL 1.1, Art/UI/Fonts/Galmuri/OFL.txt) if --font is given or found in Saved.
"""
import io
import math
import random
import sys
from pathlib import Path

import numpy as np
from PIL import Image, ImageDraw, ImageFilter

HERE = Path(__file__).resolve().parent
ARGS = sys.argv[1:]
RENDERS = Path(ARGS[ARGS.index("--renders") + 1]) if "--renders" in ARGS else HERE / "Renders"
rng = np.random.default_rng(2004)


def arr(im):
    return np.asarray(im.convert("RGB"), dtype=np.float32) / 255.0


def img(a):
    return Image.fromarray(np.clip(a * 255.0 + 0.5, 0, 255).astype(np.uint8))


def jpeg(im, q):
    b = io.BytesIO()
    im.convert("RGB").save(b, "JPEG", quality=q, subsampling=2)
    b.seek(0)
    return Image.open(b).convert("RGB")


def grade(a, lift, gain, gamma, cast):
    a = np.clip(a, 0, 1) ** gamma
    a = lift + a * (gain - lift)
    return a * np.array(cast, dtype=np.float32)


def sensor_noise(a, amount, chroma, blotch=2.0):
    h, w, _ = a.shape
    lum = a.mean(axis=2, keepdims=True)
    shadow = 1.0 - np.clip(lum * 1.6, 0, 1) * 0.7          # cheap sensors: noise mostly in the dark
    n = rng.normal(0, amount, (h, w, 1)).astype(np.float32)
    c = rng.normal(0, chroma, (h, w, 3)).astype(np.float32)
    c = np.asarray(Image.fromarray(np.clip((c + 0.5) * 255, 0, 255).astype(np.uint8)).filter(
        ImageFilter.GaussianBlur(blotch)), dtype=np.float32) / 255.0 - 0.5
    return a + (n + c * 2.2) * shadow


def vignette(a, k):
    h, w, _ = a.shape
    y, x = np.mgrid[0:h, 0:w].astype(np.float32)
    r = np.sqrt(((x - w / 2) / (w / 2)) ** 2 + ((y - h / 2) / (h / 2)) ** 2)
    return a * (1.0 - k * np.clip(r - 0.35, 0, 1) ** 1.6)[..., None]


SEG = {"0": "abcdef", "1": "bc", "2": "abged", "3": "abgcd", "4": "fgbc", "5": "afgcd", "6": "afgedc",
       "7": "abc", "8": "abcdefg", "9": "abcdfg"}


def date_stamp(im, text, scale=1.0, color=(255, 140, 40)):
    """Orange 7-segment date imprint (bottom right), like an early digital camera."""
    w, h = im.size
    dw, dh, th = 9 * scale, 15 * scale, 2.2 * scale
    gap = 4 * scale
    layer = Image.new("L", im.size, 0)
    d = ImageDraw.Draw(layer)
    total = sum((dw + gap) if ch != " " else dw * 0.7 for ch in text) + 0
    x = w - total - 14 * scale
    y = h - dh - 12 * scale
    segs = {"a": (0, 0, 1, 0), "b": (1, 0, 1, 0.5), "c": (1, 0.5, 1, 1), "d": (0, 1, 1, 1), "e": (0, 0.5, 0, 1),
            "f": (0, 0, 0, 0.5), "g": (0, 0.5, 1, 0.5)}
    for ch in text:
        if ch == " ":
            x += dw * 0.7
            continue
        if ch == "'":
            d.line([(x + dw * 0.3, y), (x + dw * 0.1, y + dh * 0.25)], fill=255, width=max(1, int(th)))
            x += dw * 0.6
            continue
        for s in SEG[ch]:
            x0, y0, x1, y1 = segs[s]
            d.line([(x + x0 * dw, y + y0 * dh), (x + x1 * dw, y + y1 * dh)], fill=255, width=max(1, int(round(th))))
        x += dw + gap
    glow = layer.filter(ImageFilter.GaussianBlur(2.0 * scale))
    base = arr(im)
    m = np.asarray(layer, dtype=np.float32)[..., None] / 255.0
    g = np.asarray(glow, dtype=np.float32)[..., None] / 255.0
    col = np.array(color, dtype=np.float32) / 255.0
    base = base * (1 - m * 0.9) + col * m * 0.95 + col * g * 0.35
    return img(base)


def camera_photo(src, crop, cast, lift=0.03, gain=1.0, gamma=1.0, noise=0.035, chroma=0.03, vig=0.45, stamp="'04 11 04",
                 mark=None, size=(640, 480)):
    im = Image.open(src).convert("RGB")
    if crop:
        im = im.crop(crop)
    im = im.resize(size, Image.LANCZOS)
    if mark:
        im = mark(im)
    a = grade(arr(im), lift, gain, gamma, cast)
    a = vignette(a, vig)
    a = sensor_noise(a, noise, chroma)
    im = img(a).filter(ImageFilter.GaussianBlur(0.6)).filter(ImageFilter.UnsharpMask(radius=1.6, percent=90, threshold=2))
    if stamp:
        im = date_stamp(im, stamp, scale=size[0] / 640.0)
    return jpeg(im, 38)


def thumb(im, size, q=60):
    t = im.resize(size, Image.LANCZOS).filter(ImageFilter.UnsharpMask(radius=0.8, percent=60, threshold=1))
    return jpeg(t, q)


# ---------------------------------------------------------------------------------------------------------------------
# the brand: a rough ring with an open eye in it (same symbol on the wrist and on the site's stamp cells)
# ---------------------------------------------------------------------------------------------------------------------


def rough_mask(size, draw_fn, roughness=0.5, seed=1, blur=0.8):
    """Draw at 4x, then break the edges with noise like ink or a burn."""
    s4 = size * 4
    m = Image.new("L", (s4, s4), 0)
    draw_fn(ImageDraw.Draw(m), s4)
    r = np.random.default_rng(seed)
    n = r.random((s4 // 8, s4 // 8)).astype(np.float32)
    n = np.asarray(Image.fromarray((n * 255).astype(np.uint8)).resize((s4, s4), Image.BICUBIC), dtype=np.float32) / 255
    fine = r.random((s4, s4)).astype(np.float32)
    mm = np.asarray(m.filter(ImageFilter.GaussianBlur(s4 * 0.012)), dtype=np.float32) / 255
    v = mm + (n - 0.5) * roughness + (fine - 0.5) * 0.25
    out = (v > 0.5).astype(np.float32)
    # ink: some lighter spots inside
    speck = (r.random((s4 // 4, s4 // 4)) > 0.93).astype(np.float32)
    speck = np.asarray(Image.fromarray((speck * 255).astype(np.uint8)).resize((s4, s4), Image.BILINEAR), dtype=np.float32) / 255
    out = out * (1 - speck * 0.5)
    o = Image.fromarray((out * 255).astype(np.uint8)).filter(ImageFilter.GaussianBlur(blur * 4 / 2))
    return o.resize((size, size), Image.LANCZOS)


def draw_ring(d, s, width=0.085):
    c = s / 2
    r0 = s * 0.46
    d.ellipse([c - r0, c - r0, c + r0, c + r0], fill=255)
    r1 = r0 - s * width
    d.ellipse([c - r1, c - r1, c + r1, c + r1], fill=0)


def draw_eye(d, s, scale=0.55):
    c = s / 2
    w = s * scale / 2
    h = w * 0.55
    pts_top = [(c - w + 2 * w * t, c - h * math.sin(math.pi * t)) for t in np.linspace(0, 1, 40)]
    pts_bot = [(c + w - 2 * w * t, c + h * math.sin(math.pi * t)) for t in np.linspace(0, 1, 40)]
    d.polygon(pts_top + pts_bot, fill=255)
    inner = 0.72
    pts_top = [(c - w * inner + 2 * w * inner * t, c - h * inner * math.sin(math.pi * t)) for t in np.linspace(0, 1, 40)]
    pts_bot = [(c + w * inner - 2 * w * inner * t, c + h * inner * math.sin(math.pi * t)) for t in np.linspace(0, 1, 40)]
    d.polygon(pts_top + pts_bot, fill=0)
    pr = h * 0.62
    d.ellipse([c - pr, c - pr, c + pr, c + pr], fill=255)


def brand_symbol(d, s):
    draw_ring(d, s, 0.07)
    draw_eye(d, s, 0.56)


def burn_symbol(d, s):
    draw_ring(d, s, 0.11)
    draw_eye(d, s, 0.6)


def white_alpha(mask):
    out = Image.new("RGBA", mask.size, (255, 255, 255, 0))
    out.putalpha(mask)
    return out


# ---------------------------------------------------------------------------------------------------------------------
# outputs
# ---------------------------------------------------------------------------------------------------------------------


def wrist_mark(points):
    def apply(im):
        sx = im.width / 800.0
        sy = im.height / 600.0
        wx, wy, _ = points["wrist"]
        ex, ey, _ = points["elbow"]
        ang = math.degrees(math.atan2(ey - wy, ex - wx))
        size = int(78 * sx)
        m = rough_mask(size, burn_symbol, roughness=0.7, seed=5, blur=1.6)
        m = m.resize((size, int(size * 0.72)), Image.LANCZOS).rotate(-ang, expand=True, resample=Image.BICUBIC)
        burn = m.filter(ImageFilter.GaussianBlur(1.2 * sx))
        halo = m.filter(ImageFilter.GaussianBlur(7 * sx))
        a = arr(im)
        cx, cy = int(wx * sx + 8 * sx), int(wy * sy)
        x0, y0 = cx - m.width // 2, cy - m.height // 2
        def put(layer, color, k, mode):
            L = np.zeros(a.shape[:2], dtype=np.float32)
            lw, lh = layer.size
            L[y0:y0 + lh, x0:x0 + lw] = np.asarray(layer, dtype=np.float32)[: a.shape[0] - y0, : a.shape[1] - x0] / 255
            L = L[..., None] * k
            col = np.array(color, dtype=np.float32)
            if mode == "add":
                return a + col * L * 0.5
            return a * (1 - L) + (a * col if mode == "mul" else col) * L
        a = put(halo, (1.0, 0.55, 0.5), 0.75, "mul")        # inflamed skin around it
        a = put(burn, (0.3, 0.05, 0.04), 0.92, "mul")       # the burned mark
        a = put(m.filter(ImageFilter.GaussianBlur(0.6)), (0.62, 0.2, 0.16), 0.35, "add")   # raw, slightly shiny
        return img(a)
    return apply


def main():
    import json
    out = HERE
    photos = {}
    photos["window"] = camera_photo(RENDERS / "window.png", (40, 0, 800, 570), (1.08, 0.96, 0.8), lift=0.035, gain=1.25,
                                    gamma=0.85, noise=0.04, chroma=0.035)
    photos["corner"] = camera_photo(RENDERS / "corner.png", None, (0.88, 1.02, 0.96), lift=0.03, gain=1.0, gamma=1.15,
                                    noise=0.035, chroma=0.03, stamp="'04 10 29")
    photos["forest"] = camera_photo(RENDERS / "forest.png", (40, 30, 760, 570), (0.98, 1.0, 0.97), lift=0.01, gain=1.3,
                                    gamma=1.12, noise=0.04, chroma=0.03, vig=0.7, stamp="'04 08 15")
    pts = json.loads((RENDERS / "wrist.json").read_text())
    photos["wrist"] = camera_photo(RENDERS / "wrist.png", None, (1.06, 0.98, 0.9), lift=0.02, gain=1.05, gamma=1.1,
                                   noise=0.03, chroma=0.025, vig=0.6, stamp="'04 11 05", mark=wrist_mark(pts))
    for k, im in photos.items():
        im.save(RENDERS / ("photo_" + k + ".jpg"), quality=95)
        thumb(im, (128, 96)).save(out / ("T_IW_Photo_" + k.capitalize() + ".png"))
    # banner photo strip (right part of the title banner): left edge fades out
    b = Image.open(RENDERS / "banner.png").convert("RGB")
    b = b.resize((470, 84), Image.LANCZOS)
    a = grade(arr(b), 0.03, 1.3, 0.85, (1.08, 0.95, 0.8))
    a = sensor_noise(a, 0.03, 0.025, 1.2)
    b = jpeg(img(a), 55)
    alpha = np.ones((84, 470), dtype=np.float32)
    ramp = np.clip(np.arange(470) / 150.0, 0, 1) ** 1.4
    alpha *= ramp[None, :]
    ba = b.convert("RGBA")
    ba.putalpha(Image.fromarray((alpha * 255).astype(np.uint8)))
    ba.save(out / "T_IW_BannerPhoto.png")
    # stamp textures (white, tinted in UMG): rough ring, rough disc, symbol
    white_alpha(rough_mask(128, lambda d, s: draw_ring(d, s, 0.075), roughness=0.45, seed=11)).save(out / "T_IW_StampRing.png")
    white_alpha(rough_mask(128, lambda d, s: d.ellipse([s * 0.04, s * 0.04, s * 0.96, s * 0.96], fill=255),
                           roughness=0.4, seed=12)).save(out / "T_IW_StampDisc.png")
    white_alpha(rough_mask(128, brand_symbol, roughness=0.4, seed=13)).save(out / "T_IW_Brand.png")
    print("wrote", sorted(p.name for p in out.glob("T_IW_*.png")))


main()
