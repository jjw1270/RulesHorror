# Makes the small UI textures for the in-game computer (monitor OS chrome + i-wish page).
# usage: python make_os_textures.py   -> writes PNGs next to this file (import to /Game/UI/Office/Monitor/Textures)
# Colors: light warm gray chrome (classic 2000-era desktop look; readable on the in-game monitor).
import os
from PIL import Image, ImageDraw

OUT = os.path.dirname(os.path.abspath(__file__))

HI = (252, 250, 245)      # highlight
LIGHT = (228, 224, 215)   # light edge
FACE = (210, 206, 196)    # button / bar face
SHADOW = (134, 129, 119)  # shadow edge
DARK = (40, 38, 35)       # dark edge
FIELD = (255, 254, 250)   # text field
ACTIVE = (226, 222, 213)  # pressed tab face


def save(img, name):
    img.save(os.path.join(OUT, name + '.png'))
    print('wrote', name)


def bevel(size, tl_out, tl_in, br_in, br_out, center, width=2):
    # 9-slice source: outer/inner top-left and bottom-right edges around a flat center
    im = Image.new('RGBA', (size, size), center + (255,))
    px = im.load()
    for i in range(size):
        for j in range(size):
            if width == 2:
                if i == size - 1 or j == size - 1:
                    c = br_out
                elif i == 0 or j == 0:
                    c = tl_out
                elif i == size - 2 or j == size - 2:
                    c = br_in
                elif i == 1 or j == 1:
                    c = tl_in
                else:
                    continue
            else:
                if i == size - 1 or j == size - 1:
                    c = br_out
                elif i == 0 or j == 0:
                    c = tl_out
                else:
                    continue
            px[i, j] = c + (255,)
    return im


def gradient(c0, c1, w=128):
    im = Image.new('RGBA', (w, 1))
    for x in range(w):
        t = x / (w - 1)
        im.putpixel((x, 0), tuple(int(c0[k] + (c1[k] - c0[k]) * t) for k in range(3)) + (255,))
    return im


def glyph(rows):
    # white-on-transparent glyph, tinted in UMG
    h, w = len(rows), len(rows[0])
    im = Image.new('RGBA', (w, h), (0, 0, 0, 0))
    for y, r in enumerate(rows):
        for x, ch in enumerate(r):
            if ch == '#':
                im.putpixel((x, y), (255, 255, 255, 255))
    return im


def globe(n):
    s = 8
    big = Image.new('RGBA', (n * s, n * s), (0, 0, 0, 0))
    d = ImageDraw.Draw(big)
    m = s
    d.ellipse((m, m, n * s - m - 1, n * s - m - 1), fill=(78, 104, 116, 255), outline=(30, 40, 46, 255), width=s)
    cx = n * s // 2
    d.ellipse((cx - n * s // 6, m, cx + n * s // 6, n * s - m - 1), outline=(150, 178, 186, 255), width=max(4, s // 2))
    d.line((m, cx, n * s - m, cx), fill=(150, 178, 186, 255), width=max(4, s // 2))
    d.arc((m, m - n * s // 5, n * s - m, cx + n * s // 8), 20, 160, fill=(150, 178, 186, 255), width=max(4, s // 2))
    d.arc((m, cx - n * s // 8, n * s - m, n * s - m + n * s // 5), 200, 340, fill=(150, 178, 186, 255), width=max(4, s // 2))
    return big.resize((n, n), Image.LANCZOS)


def monitor16():
    im = Image.new('RGBA', (16, 16), (0, 0, 0, 0))
    d = ImageDraw.Draw(im)
    d.rectangle((1, 1, 14, 11), fill=(60, 56, 50, 255), outline=(30, 28, 25, 255))
    d.rectangle((3, 3, 12, 9), fill=(92, 120, 126, 255))
    d.point((4, 4), fill=(170, 196, 200, 255))
    d.rectangle((6, 12, 9, 13), fill=(60, 56, 50, 255))
    d.rectangle((3, 14, 12, 14), fill=(30, 28, 25, 255))
    return im


def warn32():
    s = 8
    n = 32
    big = Image.new('RGBA', (n * s, n * s), (0, 0, 0, 0))
    d = ImageDraw.Draw(big)
    d.ellipse((2 * s, 2 * s, 30 * s, 30 * s), fill=(122, 20, 24, 255), outline=(40, 8, 10, 255), width=s)
    d.rectangle((14 * s, 7 * s, 18 * s, 19 * s), fill=(240, 228, 210, 255))
    d.rectangle((14 * s, 22 * s, 18 * s, 26 * s), fill=(240, 228, 210, 255))
    return big.resize((n, n), Image.LANCZOS)


def tile():
    # outer page background: faint diagonal weave (low contrast so it does not shimmer on the CRT)
    a, b = (128, 118, 103), (137, 127, 111)
    im = Image.new('RGBA', (8, 8), a + (255,))
    for i in range(8):
        im.putpixel((i, i), b + (255,))
        im.putpixel(((i + 4) % 8, i), b + (255,))
    return im


if __name__ == '__main__':
    save(bevel(8, LIGHT, HI, SHADOW, DARK, FACE), 'T_OS_Raised')
    save(bevel(8, SHADOW, DARK, LIGHT, HI, FACE), 'T_OS_Sunken')
    save(bevel(8, SHADOW, DARK, LIGHT, HI, ACTIVE), 'T_OS_Pressed')
    save(bevel(8, SHADOW, DARK, LIGHT, HI, FIELD), 'T_OS_Field')
    save(bevel(4, SHADOW, None, None, HI, FACE, width=1), 'T_OS_Thin')
    save(gradient((100, 16, 20), (184, 142, 132)), 'T_OS_TitleActive')
    save(gradient((128, 124, 116), (196, 192, 184)), 'T_OS_TitleInactive')
    save(glyph(['........', '........', '........', '........', '........', '######..', '######..']), 'T_OS_GlyphMin')
    save(glyph(['#########', '#########', '#.......#', '#.......#', '#.......#', '#.......#', '#.......#', '#########']), 'T_OS_GlyphMax')
    save(glyph(['..######', '..######', '..#....#', '######.#', '######.#', '#....###', '#....#..', '#....#..', '######..']), 'T_OS_GlyphRestore')
    save(glyph(['##....##', '.##..##.', '..####..', '...##...', '..####..', '.##..##.', '##....##']), 'T_OS_GlyphClose')
    save(glyph(['...#.....', '...##....', '...###...', '#######..', '########.', '#######..', '...###...', '...##....', '...#.....']), 'T_OS_GlyphArrow')
    save(globe(16), 'T_OS_IconInternet16')
    save(globe(32), 'T_OS_IconInternet32')
    save(monitor16(), 'T_OS_IconStart16')
    save(warn32(), 'T_OS_IconWarn32')
    save(tile(), 'T_IW_Tile')
