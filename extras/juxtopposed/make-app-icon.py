# Copyright (C) 2026 VLC authors and VideoLAN
# SPDX-License-Identifier: GPL-2.0-or-later
"""The icon of vlc.exe (and of its taskbar button, shortcuts and files),
drawn from a vector at every size Windows asks for.

  share/icons/scalable/vlc.svg   the vector: VideoLAN's cone
                                 (extras/package/macosx/asset_sources/vlc_app_icon.svg)
                                 framed as the cone of share/icons/vlc.ico
  share/icons/vlc.ico            16, 20, 24, 32, 40, 48, 64, 96, 128 (32-bit BMP)
                                 and 256 (PNG), each drawn from the vector at
                                 its size, not scaled from one picture

bin/vlc_win32_rc.rc.in puts vlc.ico in vlc.exe; the Qt interface takes the
SVG for its window icon (qt.cpp, :/logo/vlc.svg). Another artwork:
--source FILE.svg (its view box is used as is).

    python extras/juxtopposed/make-app-icon.py [--source FILE.svg]

Needs Pillow, numpy and resvg-py (pip install resvg-py).
"""
import argparse
import io
import os
import re
import struct

import numpy as np
import resvg_py
from PIL import Image

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..'))
CONE = os.path.join(ROOT, 'extras', 'package', 'macosx', 'asset_sources', 'vlc_app_icon.svg')
SVG = os.path.join(ROOT, 'share', 'icons', 'scalable', 'vlc.svg')
ICO = os.path.join(ROOT, 'share', 'icons', 'vlc.ico')
SIZES = [16, 20, 24, 32, 40, 48, 64, 96, 128, 256]
# The cone of vlc.ico fills 96 % of its height; the vector's cone spans
# x 50-463, y 30-498 of its 512 x 512 view box
FRAME = (13, 20.5, 487, 487)


def framed(svg):
    """The cone's SVG with a view box around the cone, 256 x 256 by default"""
    head = re.search(r'<svg\b[^>]*>', svg).group(0)
    new = re.sub(r'\bwidth="[^"]*"', 'width="256"', head)
    new = re.sub(r'\bheight="[^"]*"', 'height="256"', new)
    new = re.sub(r'\bviewBox="[^"]*"', 'viewBox="%g %g %g %g"' % FRAME, new)
    return svg.replace(head, new, 1)


def render(svg, size):
    data = bytes(resvg_py.svg_to_bytes(svg_string=svg, width=size, height=size))
    return Image.open(io.BytesIO(data)).convert('RGBA')


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


def png_entry(img):
    out = io.BytesIO()
    img.save(out, 'PNG', optimize=True)
    return out.getvalue()


def write_ico(path, images):
    entries = [(img.size[0], png_entry(img) if img.size[0] >= 256 else bmp_entry(img)) for img in images]
    offset = 6 + 16 * len(entries)
    out = struct.pack('<HHH', 0, 1, len(entries))
    for size, data in entries:
        out += struct.pack('<BBBBHHII', size % 256, size % 256, 0, 0, 1, 32, len(data), offset)
        offset += len(data)
    with open(path, 'wb') as f:
        f.write(out + b''.join(d for _, d in entries))


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument('--source', help='another vector to draw (its view box as is)')
    args = parser.parse_args()
    if args.source:
        svg = open(args.source, encoding='utf-8').read()
    else:
        svg = framed(open(CONE, encoding='utf-8').read())
        os.makedirs(os.path.dirname(SVG), exist_ok=True)
        with open(SVG, 'w', encoding='utf-8', newline='\n') as f:
            f.write(svg)
    write_ico(ICO, [render(svg, s) for s in SIZES])
    print('wrote', ICO, 'sizes', SIZES)


if __name__ == '__main__':
    main()
