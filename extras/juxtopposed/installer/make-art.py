# Copyright (C) 2026 VLC authors and VideoLAN
# SPDX-License-Identifier: GPL-2.0-or-later
"""Draw the installer in the redesign's look: its surfaces, its default
orange accent, its buttons, toggles and fields, and Inter. Writes under
art/ (next to this script):

  setup.ico, mark-256.png      the installer's icon
  ui/<name>-<n>.png            the pieces of the wizard, one per size n
  fonts/InterSetup-*.ttf       Inter for the wizard's text

The wizard's client area is 596 x 432 at 100 % DPI and grows with the
font, not exactly with the DPI (Inno Setup: WizardBackImageFile), so
every piece is drawn for each of these sizes; the installer picks the one
of its window. Needs Pillow and fontTools.

    python make-art.py
"""
import os
from PIL import Image, ImageDraw, ImageFilter, ImageFont, ImageOps

HERE = os.path.dirname(os.path.abspath(__file__))
OUT = os.path.join(HERE, 'art')
FONT = os.path.join(HERE, '..', '..', '..', 'modules', 'gui', 'qt', 'pixmaps', 'fonts', 'InterVariable.ttf')

# the wizard's client area at each DPI step (100 % ... 250 %)
SIZES = [(596, 432), (796, 576), (994, 720), (1193, 864), (1272, 922), (1471, 1066), (1630, 1148)]
BASE_W, BASE_H = SIZES[0]

# VLCStyle.redesign
BACK = (14, 14, 14)            # #0E0E0E, the library's background
CARD = (25, 25, 25)            # #191919, its sidebar and cards
WELL = (9, 9, 9)               # #090909, fields and toggles
BUTTON = (27, 27, 27)          # #1B1B1B, neutral button
BUTTON_HOVER = (34, 34, 34)    # #222222
ACCENT = (211, 121, 24)        # #D37918
ACCENT_DIM = (171, 99, 20)     # #AB6314
ACCENT_BACK = (55, 31, 6)      # #371F06
ACCENT_HOVER = (63, 36, 7)     # #371F06 lighter by 1.15, as RedesignButton
OFF_LABEL = (25, 25, 25)       # toggle: OFF label, #191919
OFF_SQUARE = (64, 64, 64)      # toggle: OFF square, #404040
OFF_TEXT = (107, 107, 107)     # #6B6B6B

# The layout, in the units of the 100 % size (see vlc-redesign.iss)
SIDEBAR = (10, 10, 160, 412, 10)
PIECES = {
    # name: (width, height, radius, fill, border colour, border width)
    'pill': (144, 32, 6, ACCENT_BACK, None, 0),
    'card-credit': (390, 120, 10, CARD, None, 0),
    'card-tip': (390, 92, 10, CARD, None, 0),
    'card-finish': (390, 112, 10, CARD, None, 0),
    'well': (282, 36, 6, WELL, CARD, 2),
    'track': (390, 6, 3, CARD, None, 0),
    'fill': (390, 6, 3, ACCENT, None, 0),
}
BUTTONS = {
    # name: (width, fill, hover fill)
    'button': (100, BUTTON, BUTTON_HOVER),
    'button-wide': (180, BUTTON, BUTTON_HOVER),
    'accent': (100, ACCENT_BACK, ACCENT_HOVER),
    # on a card (#191919), where the neutral button would not show
    'accent-wide': (180, ACCENT_BACK, ACCENT_HOVER),
}

SS = 4  # supersampling


def font(size, weight):
    f = ImageFont.truetype(FONT, size)
    try:
        f.set_variation_by_axes([weight])
    except Exception:
        pass
    return f


def rounded(w, h, r, fill, border=None, bw=0, alpha=255):
    """A rounded rectangle of w x h pixels, antialiased."""
    img = Image.new('RGBA', (w * SS, h * SS), (0, 0, 0, 0))
    d = ImageDraw.Draw(img)
    rr = max(0, r * SS)
    if border and bw:
        d.rounded_rectangle((0, 0, w * SS - 1, h * SS - 1), rr, fill=border + (alpha,))
        i = bw * SS
        d.rounded_rectangle((i, i, w * SS - 1 - i, h * SS - 1 - i), max(0, rr - i), fill=fill + (alpha,))
    else:
        d.rounded_rectangle((0, 0, w * SS - 1, h * SS - 1), rr, fill=fill + (alpha,))
    return img.resize((w, h), Image.LANCZOS)


def mark(size):
    """The installer's own mark: an orange play glyph in a rounded card
    (not the VLC cone, which is VideoLAN's trademark)."""
    s = size * 4
    img = Image.new('RGBA', (s, s), (0, 0, 0, 0))
    d = ImageDraw.Draw(img)
    r = int(s * 0.22)
    d.rounded_rectangle((0, 0, s - 1, s - 1), r, fill=CARD + (255,), outline=(55, 55, 55, 255),
                        width=max(4, s // 64))
    cx, cy, h = s * 0.53, s * 0.5, s * 0.40
    tri = [(cx - h * 0.45, cy - h * 0.55), (cx - h * 0.45, cy + h * 0.55), (cx + h * 0.55, cy)]
    glow = Image.new('RGBA', (s, s), (0, 0, 0, 0))
    ImageDraw.Draw(glow).polygon(tri, fill=ACCENT + (150,))
    img = Image.alpha_composite(img, glow.filter(ImageFilter.GaussianBlur(s * 0.05)))
    d = ImageDraw.Draw(img)
    lw = int(s * 0.06)
    d.polygon(tri, fill=ACCENT + (255,))
    d.line(tri + [tri[0], tri[1]], fill=ACCENT + (255,), width=lw, joint='curve')
    for x, y in tri:
        d.ellipse((x - lw / 2, y - lw / 2, x + lw / 2, y + lw / 2), fill=ACCENT + (255,))
    return img.resize((size, size), Image.LANCZOS)


def background(w, h):
    """The window: the library's background and the sidebar card."""
    sx, sy = w / BASE_W, h / BASE_H
    img = Image.new('RGBA', (w, h), BACK + (255,))
    x, y, cw, ch, r = SIDEBAR
    card = rounded(round(cw * sx), round(ch * sy), round(r * min(sx, sy)), CARD)
    img.alpha_composite(card, (round(x * sx), round(y * sy)))
    return img.convert('RGB')


# The widths of the toggle's label: 33 in the design, wider for the longer
# words of other languages (the installer picks the one its words need)
TOGGLE_LABELS = (33, 39, 45)


def toggle(on, sx, sy, label=33, rtl=False):
    """The design's ON / OFF toggle (SettingsToggle.qml): a well, a label
    33 x 26 in the design (wider for longer words) and a 26 x 26 square,
    swapped by the state; right to left, mirrored. Without its word: the
    installer writes it on, in its language."""
    s = min(sx, sy)
    tw = 68 + label - 33
    w, h = round(tw * sx), round(32 * sy)
    big = Image.new('RGBA', (w * SS, h * SS), (0, 0, 0, 0))
    d = ImageDraw.Draw(big)

    def box(x, y, bw, bh, r, colour):
        d.rounded_rectangle((x * sx * SS, y * sy * SS, (x + bw) * sx * SS - 1, (y + bh) * sy * SS - 1),
                            r * s * SS, fill=colour + (255,))
    box(0, 0, tw, 32, 6, WELL)
    if on:
        box(3, 3, label, 26, 3, ACCENT_BACK)
        box(6 + label, 3, 26, 26, 3, ACCENT_DIM)
    else:
        box(32, 3, label, 26, 3, OFF_LABEL)
        box(3, 3, 26, 26, 3, OFF_SQUARE)
    img = big.resize((w, h), Image.LANCZOS)
    return ImageOps.mirror(img) if rtl else img


def write_fonts():
    """Inter Regular and SemiBold as static fonts the wizard loads for
    itself (GDI, not the variable font), renamed so they never clash with
    an installed Inter. Latin, punctuation and the arrows only."""
    from fontTools.ttLib import TTFont
    from fontTools.varLib import instancer
    from fontTools import subset
    out = os.path.join(OUT, 'fonts')
    os.makedirs(out, exist_ok=True)
    for weight, face in ((400, 'Regular'), (600, 'SemiBold')):
        f = instancer.instantiateVariableFont(TTFont(FONT), {'wght': weight, 'opsz': 14} if 'opsz' in
                                              [a.axisTag for a in TTFont(FONT)['fvar'].axes] else {'wght': weight})
        family = 'Inter Setup' if face == 'Regular' else 'Inter Setup SemiBold'
        name = f['name']
        for rec in list(name.names):
            if rec.nameID in (1, 2, 3, 4, 6, 16, 17, 21, 22, 25):
                name.removeNames(nameID=rec.nameID)
        name.setName(family, 1, 3, 1, 0x409)
        name.setName('Regular', 2, 3, 1, 0x409)
        name.setName(family, 4, 3, 1, 0x409)
        name.setName('InterSetup-' + face, 6, 3, 1, 0x409)
        name.setName('InterSetup-' + face + ';' + str(weight), 3, 3, 1, 0x409)
        f['OS/2'].usWeightClass = 400
        f['OS/2'].fsSelection = (f['OS/2'].fsSelection & ~0b1100001) | 0b1000000   # REGULAR
        f['head'].macStyle = 0
        opts = subset.Options()
        opts.name_IDs = ['*']
        opts.name_languages = ['*']
        opts.layout_features = ['kern', 'liga', 'calt', 'case', 'tnum']
        sub = subset.Subsetter(opts)
        sub.populate(unicodes=list(range(0x20, 0x250)) + list(range(0x2000, 0x2070)) +
                     list(range(0x2190, 0x2200)) + [0x22EF, 0x2318, 0x2713])
        sub.subset(f)
        f.save(os.path.join(out, 'InterSetup-' + face + '.ttf'))


def main():
    ui = os.path.join(OUT, 'ui')
    os.makedirs(ui, exist_ok=True)
    for n, (w, h) in enumerate(SIZES):
        sx, sy = w / BASE_W, h / BASE_H
        s = min(sx, sy)
        back = background(w, h)
        back.save(os.path.join(ui, f'back-{n}.png'), optimize=True)
        ImageOps.mirror(back).save(os.path.join(ui, f'back-rtl-{n}.png'), optimize=True)
        for name, (pw, ph, r, fill, border, bw) in PIECES.items():
            rounded(round(pw * sx), round(ph * sy), round(r * s), fill, border, round(bw * s)).save(
                os.path.join(ui, f'{name}-{n}.png'), optimize=True)
        for name, (bw_, fill, hover) in BUTTONS.items():
            bw2, bh = round(bw_ * sx), round(32 * sy)
            rounded(bw2, bh, round(6 * s), fill).save(os.path.join(ui, f'{name}-{n}.png'), optimize=True)
            rounded(bw2, bh, round(6 * s), hover).save(os.path.join(ui, f'{name}-hover-{n}.png'), optimize=True)
            rounded(bw2, bh, round(6 * s), fill, alpha=128).save(
                os.path.join(ui, f'{name}-off-{n}.png'), optimize=True)
        for label in TOGGLE_LABELS:
            for rtl in (False, True):
                for on in (True, False):
                    name = f'toggle-{"on" if on else "off"}-{label}{"-rtl" if rtl else ""}'
                    toggle(on, sx, sy, label, rtl).save(os.path.join(ui, f'{name}-{n}.png'), optimize=True)
        mark(round(28 * s)).save(os.path.join(ui, f'mark-{n}.png'), optimize=True)
    write_fonts()
    sizes = [16, 20, 24, 32, 40, 48, 64, 128, 256]
    big = mark(256)
    big.save(os.path.join(OUT, 'setup.ico'), sizes=[(s, s) for s in sizes])
    big.save(os.path.join(OUT, 'mark-256.png'))
    print('art written to', OUT)


if __name__ == '__main__':
    main()
