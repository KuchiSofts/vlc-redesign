# Copyright (C) 2026 VLC authors and VideoLAN
# SPDX-License-Identifier: GPL-2.0-or-later
"""The app icon of VLC Redesign on every platform, drawn from the designer's
vector at every size each platform asks for.

The icon is Juxtopposed's logo from the VLC Redesign Figma file (the orange
cone, Frame 4 / the Icon set 42:356, 24 x 24 units, Dark/Accent/Logo
#D37918), exported from Figma as design/icons/svg/logo.svg. It is used as
the designer drew it: the cone alone, on no card, at the designer's own
padding (the 24-unit frame).

  share/icons/scalable/vlc.svg     the vector (the logo's paths unchanged)
  extras/juxtopposed/macos/AppIcon.svg   the same vector, for the macOS build
  share/icons/vlc.ico              16, 20, 24, 32, 40, 48, 64, 96, 128 (32-bit
                                   BMP) and 256 (PNG): vlc.exe, its taskbar
                                   button, shortcuts and the files it opens
  extras/juxtopposed/macos/AppIcon.icns  16 ... 512@2x (1024)
  share/icons/NxN/vlc.png          16, 22, 24, 32, 48, 64, 128, 256, 512
                                   (Linux hicolor; the Qt resources)
  share/icons/16x16|32x32/vlc.xpm  (the skins2 X11 window icon)
  share/vlc512x512.png, share/upnp_server/vlc.png, vlc512x512.png
                                   (the UPnP server's icons, seen by TVs)
  share/lua/http/images/vlc-48.png, vlc16x16.png (the web interface)
  site/favicon.ico, site/apple-touch-icon.png  (the site; logo.svg is the
                                   designer's own export)

Sizes of 40 px and under are hinted: the vector is moved by less than a
pixel and scaled by a few percent until its edges and the gaps between the
stripes fall on pixel edges (the sharpest of the candidates), so the cone
stays crisp at 16, 20, 24 and 32 px. Larger sizes are the vector as is.
The installer's icon (installer/make-art.py) uses icon() of this script.

    python extras/juxtopposed/make-app-icon.py

Needs Pillow, numpy and resvg-py (pip install pillow numpy resvg-py).
"""
import hashlib
import io
import os
import re
import struct

import numpy as np
import resvg_py
from PIL import Image

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..'))
LOGO = os.path.join(ROOT, 'design', 'icons', 'svg', 'logo.svg')
# logo.svg as Figma exported it (design/icons/index.json, the M1 assets)
LOGO_SHA256 = 'b28cdbc9bd29fbfb9259d7c635491b5ffdefe32b8df0d4156c7b400b86a7cff5'
ICO_SIZES = [16, 20, 24, 32, 40, 48, 64, 96, 128, 256]
HICOLOR = [16, 22, 24, 32, 48, 64, 128, 256, 512]
HINT_MAX = 40
# icns types: (OSType, pixels) - all PNG-encoded (macOS 10.7 and later)
ICNS = [(b'icp4', 16), (b'ic11', 32), (b'icp5', 32), (b'ic12', 64), (b'icp6', 64),
        (b'ic07', 128), (b'ic13', 256), (b'ic08', 256), (b'ic14', 512), (b'ic09', 512),
        (b'ic10', 1024)]


def logo_paths():
    """The designer's paths: [(d, fill)]"""
    data = open(LOGO, 'rb').read()
    if hashlib.sha256(data).hexdigest() != LOGO_SHA256:
        print('note: logo.svg differs from the Figma export it was checked against')
    svg = data.decode('utf-8')
    return re.findall(r'<path\b[^>]*?\bd="([^"]+)"[^>]*?\bfill="([^"]+)"', svg)


def vector(width=512, comment=None, transform=None):
    """The logo in its 24-unit frame, drawn at width x width"""
    out = ['<svg width="%d" height="%d" viewBox="0 0 24 24" fill="none" '
           'xmlns="http://www.w3.org/2000/svg">' % (width, width)]
    if comment:
        out.append('<!-- %s -->' % comment)
    if transform:
        out.append('<g transform="%s">' % transform)
    out += ['<path d="%s" fill="%s"/>' % p for p in logo_paths()]
    if transform:
        out.append('</g>')
    out.append('</svg>')
    return '\n'.join(out) + '\n'


def render_svg(svg, size):
    data = bytes(resvg_py.svg_to_bytes(svg_string=svg, width=size, height=size))
    return Image.open(io.BytesIO(data)).convert('RGBA')


def _fits(img, pad):
    a = np.asarray(img)[:, :, 3]
    return not (a[:pad].any() or a[-pad:].any() or a[:, :pad].any() or a[:, -pad:].any())


def hinted(size):
    """The vector at size, its edges moved onto the pixel grid: of the
    candidate offsets (under a pixel) and scales (97 to 103 %), the one with
    the fewest half-covered pixels that still fits the square"""
    pad = 2
    unit = 24 / size                      # one pixel in the logo's units
    best = None
    for k in (1.0, 0.98, 1.02, 0.97, 1.03):
        for iy in range(-4, 5):
            for ix in range(-4, 5):
                dx, dy = ix / 8 * unit, iy / 8 * unit
                # scaled around the frame's centre, then moved
                tr = 'translate(%g %g) translate(12 12) scale(%g) translate(-12 -12)' % (dx, dy, k)
                big = size + 2 * pad
                svg = vector(big, transform='translate(%g %g) %s' % (pad * unit, pad * unit, tr))
                svg = svg.replace('viewBox="0 0 24 24"', 'viewBox="0 0 %g %g"' % (big * unit, big * unit))
                img = render_svg(svg, big)
                if not _fits(img, pad):
                    continue
                a = np.asarray(img)[:, :, 3].astype(np.float64) / 255
                cover = a.sum()
                blur = (4 * a * (1 - a)).sum() / max(cover, 1)
                score = blur + 0.02 * (abs(ix) + abs(iy)) / 4 + 0.6 * abs(k - 1)
                if best is None or score < best[0]:
                    best = (score, img.crop((pad, pad, pad + size, pad + size)))
    return best[1]


_cache = {}


def icon(size):
    """The app icon at size px (RGBA): hinted when small"""
    if size not in _cache:
        _cache[size] = hinted(size) if size <= HINT_MAX else render_svg(vector(size), size)
    return _cache[size]


def png_bytes(img):
    out = io.BytesIO()
    img.save(out, 'PNG', optimize=True)
    return out.getvalue()


def bmp_entry(img):
    """A 32-bit DIB as an icon stores it: bottom-up BGRA, height doubled, and
    its 1-bit AND mask (transparent where alpha is 0)"""
    w, h = img.size
    a = np.asarray(img)
    bgra = a[::-1, :, [2, 1, 0, 3]].tobytes()
    stride = ((w + 31) // 32) * 4
    mask = bytearray()
    for row in (a[::-1, :, 3] == 0):
        bits = np.packbits(row.astype(np.uint8))
        mask += bytes(bits) + bytes(stride - len(bits))
    header = struct.pack('<IiiHHIIiiII', 40, w, h * 2, 1, 32, 0, len(bgra) + len(mask), 0, 0, 0, 0)
    return header + bgra + bytes(mask)


def write_ico(path, sizes):
    entries = [(s, png_bytes(icon(s)) if s >= 256 else bmp_entry(icon(s))) for s in sizes]
    offset = 6 + 16 * len(entries)
    out = struct.pack('<HHH', 0, 1, len(entries))
    for size, data in entries:
        out += struct.pack('<BBBBHHII', size % 256, size % 256, 0, 0, 1, 32, len(data), offset)
        offset += len(data)
    save(path, out + b''.join(d for _, d in entries))


def write_icns(path):
    body = b''
    for kind, px in ICNS:
        data = png_bytes(icon(px))
        body += kind + struct.pack('>I', len(data) + 8) + data
    save(path, b'icns' + struct.pack('>I', len(body) + 8) + body)


def write_xpm(path, size):
    """An XPM (no partial transparency: alpha under 50 % is None), keeping
    the C array's name of the file it replaces"""
    name = 'vlc_xpm'
    if os.path.exists(path):
        m = re.search(r'char\s*\*\s*const\s+(\w+)\s*\[', open(path, encoding='latin-1').read())
        name = m.group(1) if m else name
    a = np.asarray(icon(size))
    colours, rows = {}, []
    chars = 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789'
    for y in range(size):
        row = ''
        for x in range(size):
            r, g, b, al = (int(v) for v in a[y, x])
            key = None if al < 128 else '#%02X%02X%02X' % (r, g, b)
            if key not in colours:
                n = len(colours)
                colours[key] = chars[n % len(chars)] + chars[n // len(chars)]
            row += colours[key]
        rows.append(row)
    out = ['/* XPM */', 'static const char *const %s[] = {' % name,
           '/* width height ncolors chars_per_pixel */', '"%d %d %d 2",' % (size, size, len(colours)),
           '/* colors */']
    out += ['"%s c %s",' % (c, 'None' if k is None else k) for k, c in colours.items()]
    out.append('/* pixels */')
    out += ['"%s"%s' % (r, ',' if i < size - 1 else '') for i, r in enumerate(rows)]
    out.append('};')
    save(path, ('\n'.join(out) + '\n').encode('ascii'))


def write_png(path, size, background=None):
    img = icon(size)
    if background:
        back = Image.new('RGBA', img.size, background)
        img = Image.alpha_composite(back, img)
    save(path, png_bytes(img))


def save(path, data):
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, 'wb') as f:
        f.write(data)
    print('wrote', os.path.relpath(path, ROOT))


def main():
    note = ("The app icon of VLC Redesign: Juxtopposed's logo from the VLC Redesign Figma file "
            "(design/icons/svg/logo.svg), its paths unchanged, in its 24-unit frame. "
            "Written by extras/juxtopposed/make-app-icon.py.")
    for path in (os.path.join(ROOT, 'share', 'icons', 'scalable', 'vlc.svg'),
                 os.path.join(ROOT, 'extras', 'juxtopposed', 'macos', 'AppIcon.svg')):
        save(path, vector(512, note).encode('utf-8'))
    write_ico(os.path.join(ROOT, 'share', 'icons', 'vlc.ico'), ICO_SIZES)
    write_icns(os.path.join(ROOT, 'extras', 'juxtopposed', 'macos', 'AppIcon.icns'))
    for s in HICOLOR:
        write_png(os.path.join(ROOT, 'share', 'icons', '%dx%d' % (s, s), 'vlc.png'), s)
    for s in (16, 32):
        write_xpm(os.path.join(ROOT, 'share', 'icons', '%dx%d' % (s, s), 'vlc.xpm'), s)
    write_png(os.path.join(ROOT, 'share', 'vlc512x512.png'), 512)
    write_png(os.path.join(ROOT, 'share', 'upnp_server', 'vlc.png'), 32)
    write_png(os.path.join(ROOT, 'share', 'upnp_server', 'vlc512x512.png'), 512)
    write_png(os.path.join(ROOT, 'share', 'lua', 'http', 'images', 'vlc-48.png'), 128)
    write_png(os.path.join(ROOT, 'share', 'lua', 'http', 'images', 'vlc16x16.png'), 16)
    write_ico(os.path.join(ROOT, 'site', 'favicon.ico'), [16, 32, 48])
    # iOS draws a transparent touch icon on black: the site's background
    write_png(os.path.join(ROOT, 'site', 'apple-touch-icon.png'), 180, background=(14, 14, 14, 255))


if __name__ == '__main__':
    main()
