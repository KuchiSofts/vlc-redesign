#!/usr/bin/env python3
# Copyright (C) 2026 VLC authors and VideoLAN
# SPDX-License-Identifier: GPL-2.0-or-later
"""SVG sources of the icon glyphs, from the Figma "Icon" component set (42:356).

The glyphs (modules/gui/qt/pixmaps/redesign/icons/<name>/<state>[@2x].png)
were exported from Figma as PNG. This tool gives each state an SVG next to
its PNG (<state>.svg), cut from an SVG export of the whole 42:356 frame
(Figma: select the frame, Export, SVG; or the REST/MCP export of the node).

    python design/tools/icon_svgs.py map FRAME.svg       # match every PNG to its symbol (icon_svgs.json)
    python design/tools/icon_svgs.py extract FRAME.svg   # write the SVGs from icon_svgs.json
    python design/tools/icon_svgs.py compare [--figma FRAME.png]
                                                         # compare them with the PNGs (icon_svgs_report.md)

`map` and `compare` render with QtSvg (PySide6 6.8, the renderer of the app)
and need numpy and Pillow; `extract` is standard library only. FRAME.png is
Figma's 1x render of the frame without its background (the screenshot of the
node), for the columns comparing with Figma's own drawing.

How a state is matched: among the symbols named as the icon, the one, the
part of it the PNG shows (a two-tone icon is split into its grey and its
accent PNG), the subpixel position it was cut at and the colour of each part
are measured on the PNG; the SVG keeps Figma's colour when the PNG has it
(within 2 levels), else the PNG's (the dimmed and disabled states are not
Figma variants). After a sync of the icons, `python design/tools/sync_assets.py`.
"""

import argparse
import json
import re
import sys
import xml.etree.ElementTree as ET
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
ICONS = ROOT / "modules" / "gui" / "qt" / "pixmaps" / "redesign" / "icons"
MAP = Path(__file__).with_name("icon_svgs.json")
REPORT = Path(__file__).with_name("icon_svgs_report.md")
QRC = ROOT / "modules" / "gui" / "qt" / "assets.qrc"
SVG_NS = "http://www.w3.org/2000/svg"
NS = "{" + SVG_NS + "}"
ET.register_namespace("", SVG_NS)

# The symbols of 42:356 in their order on the frame: 12 per row, 44 apart,
# the first at (20, 20). (Figma's export names the groups, not the nodes.)
NODES = """
42:355 2493:9293 2230:9222 2238:9232 470:507 1803:2957 1803:2965 42:357 2461:9719 858:5098 217:191 467:492
1089:4450 2493:9253 1089:4464 42:398 2461:9635 467:497 44:407 2461:9697 496:2439 44:419 999:4802 6073:13944
4004:12338 3180:17978 44:427 45:12 6073:13875 187:322 6072:13836 45:18 6073:13879 45:22 45:32 45:28
45:36 1622:3137 2461:9784 1919:9572 2154:9699 2493:9246 182:179 182:187 182:193 182:198 463:417 45:40
48:46 48:51 48:54 6031:13553 70:828 51:808 2461:9734 2139:9022 2461:9779 87:1031 467:502 69:814
6586:17393 6206:13629 6206:13653 858:5697 858:5703 87:1041 999:4806 999:4810 5886:24408 1102:5055 71:851 87:1009
470:592 71:857 87:1015 470:587 70:833 2461:9672 477:1202 477:1208 477:1096 477:1101 477:1107 87:1026
470:597 71:863 71:868 71:881 2461:9739 71:884 2461:9744 6215:13606 71:887 2461:9749 71:890 2461:9754
330:267 187:470 2460:9614 187:474 2461:9618 71:872 71:875 71:878 247:418 48:57 463:550 463:560
856:3212 856:3220 774:1038 774:1075 6063:13611 494:2394 2428:14606 4004:12348 494:2400 463:564 856:2206 856:2147
2154:9680 858:5155 2437:14784 2493:9300 858:5164 999:4914 2139:8985 2461:9651 999:4960 1088:4044 1911:8286 2461:9663
2107:6307 2461:9644 1915:8362 2461:9628 2107:6312 2461:9640 2107:6319 2150:9212 2493:9283 2150:9248 2107:6523 2107:6531
2107:6596 2115:7386 2115:7391 2115:7482 6571:16751 6073:13901 2115:7492 2127:7523 2461:9714 2127:7531 6321:14144 2127:7548
6320:13921 2127:7779 2138:8026 2139:8048 2139:8064 2139:8078 2139:8439 2146:9032 2372:8559 2372:8571 2372:8578 2372:8588
6114:13932 6114:13938 6114:13944 6114:13955 6114:13996 6114:14002 6114:14008 6114:14021 6114:14014 6114:13961 6114:13967 6114:13985
6114:13979 6114:13973 2404:9198 2404:9204 2404:9211 2404:9215 2430:14629 2511:9352 2430:14666 2437:14888 2459:9542 2459:9548
2460:9552 5884:24330 2493:9307 2493:9342 2516:9369 2516:9378 2516:9375 3046:10386 3050:10420 3050:10424 6073:14046 6073:14054
6075:14088 6075:14095 6191:13585 6202:13602
""".split()
COLUMNS, PITCH, ORIGIN, CELL = 12, 44, 20, 24

COLOUR_ATTRS = ("fill", "stroke")
# A PNG further than this from every symbol (3 x 3 averaged alpha) has none
NO_SYMBOL = 8.0
# The app's names of symbols named otherwise in Figma
ALIASES = {"sidebar": ["sidebar-left", "sidebar"], "right-sidebar": ["sidebar-right"],
           "repeat": ["repeat-right"], "shuffle": ["shuffle-right"], "spectrometer": ["spectometer"],
           "eq-flat": ["EQ"], "eq-fullbasstreble": ["eq-fullbassandtreble"], "play": ["play", "playback"]}
# Symbols Figma's SVG export does not draw as Figma does (QtSvg of the export
# against Figma's own render, 3 x 3 averaged alpha, 6 to 22 levels apart; the
# others are within 4): boolean shapes stroked as one, exported as their outline
LOSSY = {"2459:9542", "999:4810", "2107:6312", "2461:9734", "2461:9663", "1911:8286", "2107:6307"}
# Pictures drawn in this repository from a glyph, not Figma variants
MADE_HERE = {f"vumeter/{state}": "the speed glyph (2115:7386) with its needle in the accent dim, drawn in this "
             "repository for the VU meter (no Figma symbol)" for state in ("default", "default-grey", "default-accent")}
# Empty pictures: the symbols export no shape
EMPTY = {"0/default": "2107:6596", "check/default": "494:2400", "eq-flat/default-accent": "2372:8559"}


# --- the frame ---------------------------------------------------------------

def frame_symbols(frame_path):
    """{node id: {"name", "x", "y", "element"}} of the symbols the export draws"""
    root = ET.parse(frame_path).getroot()
    icon = next((g for g in root.iter(NS + "g") if g.get("id") == "Icon"), root)
    symbols = {}
    for group in icon:
        if group.tag != NS + "g" or not str(group.get("id", "")).startswith("name="):
            continue
        x, y = first_point(group)
        col, row = int((x - ORIGIN) // PITCH), int((y - ORIGIN) // PITCH)
        node = NODES[row * COLUMNS + col]
        if node in symbols:
            raise SystemExit(f"two groups in the cell of {node}")
        symbols[node] = {"name": group.get("id"), "x": ORIGIN + col * PITCH,
                         "y": ORIGIN + row * PITCH, "element": group}
    return symbols


def first_point(group):
    for e in group.iter():
        if e.get("d"):
            nums = re.findall(r"-?\d*\.?\d+(?:e-?\d+)?", e.get("d"))
            return float(nums[0]), float(nums[1])
        if e.get("cx"):
            return float(e.get("cx")), float(e.get("cy"))
        if e.get("x") and e.get("width"):
            return float(e.get("x")), float(e.get("y"))
    raise SystemExit(f"empty group {group.get('id')}")


def fmt(v):
    s = f"{v:.4f}".rstrip("0").rstrip(".")
    return "0" if s in ("-0", "") else s


def shift_path(d, dx, dy):
    """The path moved by (dx, dy): absolute commands only change"""
    out, cmd, args = [], None, []
    tokens = re.findall(r"[A-Za-z]|-?\d*\.?\d+(?:[eE][-+]?\d+)?", d)
    arity = {"M": 2, "L": 2, "T": 2, "H": 1, "V": 1, "C": 6, "S": 4, "Q": 4, "A": 7, "Z": 0}

    def flush():
        if cmd is None:
            return
        n = arity[cmd.upper()]
        if n == 0:
            out.append(cmd)
            return
        for i in range(0, len(args), n):
            chunk = [float(a) for a in args[i:i + n]]
            if cmd.isupper():
                if cmd == "H":
                    chunk[0] += dx
                elif cmd == "V":
                    chunk[0] += dy
                elif cmd == "A":
                    chunk[5] += dx
                    chunk[6] += dy
                else:
                    for j in range(0, n, 2):
                        chunk[j] += dx
                        chunk[j + 1] += dy
            out.append((cmd if i == 0 else "") + " ".join(fmt(c) for c in chunk))

    for t in tokens:
        if t.isalpha():
            flush()
            cmd, args = t, []
        else:
            args.append(t)
    flush()
    return "".join(p if not out or i == 0 or p[0].isalpha() else " " + p for i, p in enumerate(out))


def shift_element(e, dx, dy):
    if e.get("d"):
        e.set("d", shift_path(e.get("d"), dx, dy))
    for a, delta in (("cx", dx), ("cy", dy), ("x", dx), ("y", dy)):
        if e.get(a) is not None:
            e.set(a, fmt(float(e.get(a)) + delta))
    t = e.get("transform")
    if t:
        m = re.fullmatch(r"rotate\((-?[\d.]+) (-?[\d.]+) (-?[\d.]+)\)", t)
        if not m:
            raise SystemExit(f"transform not handled: {t}")
        e.set("transform", f"rotate({m.group(1)} {fmt(float(m.group(2)) + dx)} {fmt(float(m.group(3)) + dy)})")


def colour_groups(element):
    """The colours of a symbol, in the order they first appear"""
    seen = []
    for e in element.iter():
        for a in COLOUR_ATTRS:
            v = e.get(a)
            if v and v != "none" and v.upper() not in seen:
                seen.append(v.upper())
    return seen


def symbol_svg(symbol, colours=None, size=(CELL, CELL), offset=(0.0, 0.0), clean=False):
    """A standalone SVG of the symbol at (0, 0). colours: {Figma colour:
    colour, None to leave that part out, [colour, opacity] for a translucent one}"""
    import copy
    g = copy.deepcopy(symbol["element"])
    ox, oy = -symbol["x"] + offset[0], -symbol["y"] + offset[1]
    for e in list(g.iter()):
        shift_element(e, ox, oy)
        if clean and "id" in e.attrib:
            del e.attrib["id"]
    if colours is not None:
        drop = []
        for parent in g.iter():
            for child in list(parent):
                for a in COLOUR_ATTRS:
                    v = child.get(a)
                    if not v or v == "none":
                        continue
                    want = colours.get(v.upper(), v)
                    if want is None:
                        child.set(a, "none")
                    elif isinstance(want, list):
                        child.set(a, want[0])
                        if want[1] < 1:
                            child.set(a + "-opacity", fmt(want[1]))
                    else:
                        child.set(a, want)
                if all(child.get(a) in (None, "none") for a in COLOUR_ATTRS) and not list(child):
                    drop.append((parent, child))
        for parent, child in drop:
            parent.remove(child)
    # groups left with nothing to draw, and Figma's wrapper groups
    changed = True
    while changed:
        changed = False
        for parent in g.iter():
            for child in list(parent):
                if child.tag == NS + "g" and not list(child):
                    parent.remove(child)
                    changed = True
    body = list(g) if clean else [g]
    if clean:
        body = unwrap(body)
    w, h = size
    head = (f'<svg xmlns="{SVG_NS}" width="{w}" height="{h}" viewBox="0 0 {CELL} {CELL}" fill="none">')
    inner = "".join(ET.tostring(e, encoding="unicode").replace(f' xmlns="{SVG_NS}"', "").replace("ns0:", "")
                    for e in body)
    return head + inner + "</svg>"


def unwrap(elements):
    """Groups without attributes (Figma's frames) give their children"""
    out = []
    for e in elements:
        if e.tag == NS + "g" and not e.attrib:
            out += unwrap(list(e))
        else:
            out.append(e)
    return out


def pretty(svg):
    """One element per line, as the other SVG icons of the app"""
    return re.sub(r"><", ">\n<", svg).rstrip() + "\n"


# --- rendering (map, compare) ------------------------------------------------

_app = None


def render(svg, size):
    """RGBA array (straight alpha) of the SVG drawn by QtSvg at size (w, h)"""
    global _app
    import numpy as np
    from PySide6.QtCore import QByteArray, QRectF
    from PySide6.QtGui import QGuiApplication, QImage, QPainter
    from PySide6.QtSvg import QSvgRenderer
    if _app is None:
        _app = QGuiApplication.instance() or QGuiApplication(["icon_svgs", "-platform", "offscreen"])
    w, h = size
    renderer = QSvgRenderer(QByteArray(svg.encode()))
    image = QImage(w, h, QImage.Format_ARGB32_Premultiplied)
    image.fill(0)
    painter = QPainter(image)
    renderer.render(painter, QRectF(0, 0, w, h))
    painter.end()
    image = image.convertToFormat(QImage.Format_RGBA8888)
    return np.frombuffer(image.constBits(), np.uint8).reshape(h, w, 4).astype(float)


def load_png(path):
    import numpy as np
    from PIL import Image
    return np.asarray(Image.open(path).convert("RGBA")).astype(float)


def load_png_size(path):
    """(width, height) from the PNG header (standard library)"""
    import struct
    with open(path, "rb") as f:
        head = f.read(24)
    return struct.unpack(">II", head[16:24])


def premultiplied(a):
    out = a.copy()
    out[..., :3] *= a[..., 3:4] / 255.0
    return out


def png_files():
    """[(icon, state, path of the 1x PNG, path of the @2x PNG or None)]"""
    out = []
    for path in sorted(ICONS.glob("*/*.png")):
        if path.stem.endswith("@2x"):
            continue
        twice = path.with_name(path.stem + "@2x.png")
        out.append((path.parent.name, path.stem, path, twice if twice.exists() else None))
    return out


OFFSETS = [i / 8 for i in range(-4, 5)]
_cache = {}


def quick_svg(symbol, keep, size, offset=(0.0, 0.0)):
    """symbol_svg for rendering only: the parts left out drawn with "none" """
    k = (id(symbol), size, tuple(offset))
    if k not in _cache:
        _cache[k] = symbol_svg(symbol, None, size, offset)
    svg = _cache[k]
    for c, v in keep.items():
        if v is None:
            svg = re.sub(rf'(fill|stroke)="{c}"', r'\1="none"', svg, flags=re.I)
    return svg


def measure(symbol, png, subset):
    """Best subpixel offset of the symbol's parts `subset` on the PNG, and
    the colour of each part there"""
    import numpy as np
    groups = colour_groups(symbol["element"])
    h, w = png.shape[:2]
    keep = {c: (c if c in subset else None) for c in groups}
    best = None
    for dy in OFFSETS:
        for dx in OFFSETS:
            r = render(quick_svg(symbol, keep, (w, h), (dx, dy)), (w, h))
            d = shape_difference(r[..., 3], png[..., 3])
            if best is None or d < best[0]:
                best = (d, dx, dy)
    shape, dx, dy = best
    at_zero = shape_difference(render(quick_svg(symbol, keep, (w, h)), (w, h))[..., 3], png[..., 3])
    # a real offset of the PNG (an instance placed between pixels), not noise
    if max(abs(dx), abs(dy)) < 0.25 or at_zero - shape < 0.5:
        shape, dx, dy = at_zero, 0.0, 0.0
    colours = {}
    for c in groups:
        if c not in subset:
            colours[c] = None
            continue
        alone = render(quick_svg(symbol, {g: (g if g == c else None) for g in groups}, (w, h), (dx, dy)), (w, h))
        others = render(quick_svg(symbol, {g: (g if g != c and g in subset else None) for g in groups},
                                   (w, h), (dx, dy)), (w, h))
        # pixels this part covers well and the others not: the PNG's colour,
        # and its opacity as the PNG's alpha over the coverage QtSvg draws
        # (a 1.5 px stroke between pixels covers none of them fully)
        # (thin parts, as the caps of the spectrum, cover no pixel half)
        for cover, alone_only in ((128, True), (64, True), (16, True), (16, False)):
            mask = (alone[..., 3] >= cover) & (png[..., 3] > 0)
            if alone_only:
                mask &= others[..., 3] <= 5
            if mask.sum() >= 3:
                break
        if mask.sum() == 0:
            colours[c] = None
            continue
        rgb = np.median(png[..., :3][mask], axis=0)
        alpha = min(1.0, float(np.median(png[..., 3][mask] / alone[..., 3][mask])))
        if alpha >= 0.93:
            alpha = 1.0
        figma = tuple(int(c[i:i + 2], 16) for i in (1, 3, 5))
        if all(abs(rgb[i] - figma[i]) <= 2 for i in range(3)):
            hexc = c
        else:
            hexc = "#" + "".join(f"{int(round(v)):02X}" for v in rgb)
        colours[c] = hexc if alpha >= 0.97 else [hexc, round(alpha, 2)]
    return {"offset": [dx, dy], "shape": round(shape, 2), "colours": colours}


def blurred(alpha):
    """Alpha averaged over 3 x 3 pixels: the shape, without the antialiasing"""
    import numpy as np
    p = np.pad(alpha, 1)
    h, w = alpha.shape
    return sum(p[y:y + h, x:x + w] for y in range(3) for x in range(3)) / 9


def shape_difference(a, b):
    import numpy as np
    return float(np.abs(blurred(a) - blurred(b)).mean())


def label(symbol_name):
    """The app's name of a symbol: "name=arrow, direction=left" -> arrow-left"""
    a = dict(kv.split("=", 1) for kv in symbol_name.split(", "))
    if a.get("EQ", "default") != "default":
        return "eq-" + a["EQ"].replace(" ", "")
    out = a["name"].replace(" ", "-")
    if a.get("direction", "default") != "default":
        out += "-" + a["direction"]
    return out


def candidates(symbols, png, names):
    """[(shape difference, node, subset)] best first: every subset of the colours
    of each symbol named as the icon"""
    import itertools
    h, w = png.shape[:2]
    scored = []
    for node, s in symbols.items():
        if label(s["name"]) not in names or node in LOSSY:
            continue
        groups = colour_groups(s["element"])
        for n in range(1, len(groups) + 1):
            for subset in itertools.combinations(groups, n):
                keep = {c: (c if c in subset else None) for c in groups}
                r = render(quick_svg(s, keep, (w, h)), (w, h))
                scored.append((shape_difference(r[..., 3], png[..., 3]), node, list(subset), len(groups)))
    scored.sort(key=lambda t: t[0])
    return scored


def preference(symbol_name, icon, state, subset, groups):
    """What breaks a near tie: the whole symbol, its state, one tone"""
    p = 0.0
    if len(subset) < groups:
        p += 1.5
    if "two-tone=yes" in symbol_name:
        p += 1.0
    figma_state = re.search(r"state=(\w+)", symbol_name).group(1)
    if figma_state == state.split("-")[0]:
        p -= 0.5
    return p


def short(name):
    name = re.sub(r", (direction=default|state=default|value=default|EQ=default|two-tone=no)", "", name)
    return name.replace("name=", "")


def cmd_map(args):
    symbols = frame_symbols(args.frame)
    old = json.loads(MAP.read_text("utf-8")) if MAP.exists() else {}
    pinned = {k: v for k, v in old.get("states", {}).items() if v.get("pinned")}
    states = {}
    for icon, state, path, _ in png_files():
        key = f"{icon}/{state}"
        png = load_png(path)
        if key in pinned:
            entry = pinned[key]
            if entry.get("node"):
                s = symbols[entry["node"]]
                entry.update(measure(s, png, [c for c, v in entry["colours"].items() if v is not None])
                             if not entry.get("keep_colours") else {})
            states[key] = entry
            continue
        h, w = png.shape[:2]
        if w != h or w % CELL:
            states[key] = {"node": None, "why": f"{w} x {h}: the glyph with a badge of the screen it was cut "
                                               "from, not a symbol of 42:356"}
            continue
        if key in MADE_HERE:
            states[key] = {"node": None, "why": MADE_HERE[key]}
            print(f"{key:32} made here", flush=True)
            continue
        if png[..., 3].max() == 0:
            states[key] = {"node": EMPTY.get(key), "empty": True,
                           "why": "an empty picture (the Figma symbol draws nothing)"}
            print(f"{key:32} empty", flush=True)
            continue
        names = ALIASES.get(icon, [icon])
        ranked = candidates(symbols, png, names)
        if not ranked:
            lossy = [n for n in LOSSY if label(symbols[n]["name"]) in names]
            why = (f"Figma's SVG export of {', '.join(lossy)} loses its detail (a boolean shape's stroke)"
                   if lossy else "no symbol of 42:356 has this name")
            states[key] = {"node": None, "why": why}
            print(f"{key:32} none: {why}", flush=True)
            continue
        best = min(ranked, key=lambda c: c[0] + preference(symbols[c[1]]["name"], icon, state, c[2], c[3]))
        s = symbols[best[1]]
        entry = {"node": best[1], "symbol": s["name"], "match": round(best[0], 2)}
        entry.update(measure(s, png, best[2]))
        if entry["shape"] > NO_SYMBOL:
            lossy = [n for n in LOSSY if label(symbols[n]["name"]) in names]
            why = (f"Figma's SVG export of {', '.join(sorted(lossy))} loses its detail (a boolean shape's stroke)"
                   if lossy else f"another drawing than the symbols of this name (closest {best[1]}: "
                                 f"shape {entry['shape']:.1f})")
            states[key] = {"node": None, "why": why}
            print(f"{key:32} none: {why}", flush=True)
            continue
        states[key] = entry
        print(f"{key:32} {best[1]:12} {short(s['name'])[:44]:44} {best[0]:.2f}", flush=True)
    MAP.write_text(json.dumps({"$description": "Each PNG state of the icon glyphs: its symbol in Figma 42:356 "
                               "(node), the part shown (colours: Figma colour -> colour of the state, null = "
                               "left out) and the subpixel offset of the PNG. Written by icon_svgs.py map; "
                               "\"pinned\" entries are kept.", "frame": "42:356", "states": states},
                              indent=1, ensure_ascii=False) + "\n", "utf-8")
    return 0


def cmd_extract(args):
    symbols = frame_symbols(args.frame)
    data = json.loads(MAP.read_text("utf-8"))
    written = 0
    for key, entry in data["states"].items():
        icon, state = key.split("/")
        if entry.get("empty"):
            svg = f'<svg xmlns="{SVG_NS}" width="{CELL}" height="{CELL}" viewBox="0 0 {CELL} {CELL}" fill="none"></svg>'
        elif not entry.get("node"):
            continue
        else:
            size = load_png_size(ICONS / icon / f"{state}.png")
            svg = symbol_svg(symbols[entry["node"]], entry["colours"], size=size,
                             offset=tuple(entry.get("offset", (0, 0))), clean=True)
        (ICONS / icon / f"{state}.svg").write_text(pretty(svg), "utf-8", newline="\n")
        written += 1
    print(f"wrote {written} SVG files")
    return 0


def cmd_compare(args):
    import numpy as np
    data = json.loads(MAP.read_text("utf-8"))
    figma = load_png(args.figma) if args.figma else None
    rows = []
    for icon, state, path, twice in png_files():
        key = f"{icon}/{state}"
        entry = data["states"].get(key, {})
        svg_path = path.with_suffix(".svg")
        if not svg_path.exists():
            rows.append((key, None, entry.get("why", "no SVG")))
            continue
        svg = svg_path.read_text("utf-8")
        png = load_png(path)
        h, w = png.shape[:2]
        r24 = render(svg, (w, h))
        d = np.abs(premultiplied(r24) - premultiplied(png))
        res = {"max24": int(d.max()), "mean24": round(float(d.mean()), 2),
               "shape": round(shape_difference(r24[..., 3], png[..., 3]), 2)}
        both = (r24[..., 3] > 200) & (png[..., 3] > 200)
        res["colour"] = (int(np.abs(r24[..., :3][both].mean(axis=0) - png[..., :3][both].mean(axis=0)).max())
                         if both.any() else 0)
        r48 = render(svg, (2 * w, 2 * h))
        if twice is not None:
            d2 = np.abs(premultiplied(r48) - premultiplied(load_png(twice)))
            res["max48"] = int(d2.max())
            res["mean48"] = round(float(d2.mean()), 2)
        else:
            # 48 px drawing averaged back to 24: the shape the PNG shows
            down = premultiplied(r48).reshape(h, 2, w, 2, 4).mean(axis=(1, 3))
            d2 = np.abs(down - premultiplied(png))
            res["max48"] = int(d2.max())
            res["mean48"] = round(float(d2.mean()), 2)
            res["down"] = True
        res["offset"] = entry.get("offset", [0, 0])
        res["node"] = entry.get("node")
        # Figma's own drawing of the symbol (alpha: the states recolour it)
        if figma is not None and h == CELL and entry.get("colours") and None not in entry["colours"].values():
            i = NODES.index(entry["node"])
            x, y = ORIGIN + (i % COLUMNS) * PITCH, ORIGIN + (i // COLUMNS) * PITCH
            cell = figma[y:y + h, x:x + w, 3]
            res["figma"] = round(float(np.abs(cell - png[..., 3]).mean()), 2)
            res["qtfigma"] = round(float(np.abs(cell - r24[..., 3]).mean()), 2)
        rows.append((key, res, ""))
    write_report(rows)
    if args.json:
        Path(args.json).write_text(json.dumps(rows, indent=1), "utf-8")
    return 0


def verdict(res):
    """exact: the PNG is Figma's own render of the symbol (alpha within 1
    level on average), so the SVG is its source as drawn; same: the shape (3 x 3
    averaged alpha within 3 levels) and the colour (within 6) of the PNG, the
    antialiasing of another rasteriser; close: the rest"""
    if res.get("figma") is not None and res["figma"] <= 1.0 and res["colour"] <= 6:
        return "exact"
    if res["shape"] <= 3.0 and res["colour"] <= 6:
        return "same"
    return "close"


CLASSES = {"exact": "Exact: the PNG is Figma's render of the symbol",
           "same": "Same drawing: shape and colour of the PNG, another rasteriser",
           "close": "Close: the symbol, its PNG drawn heavier or lighter"}


def write_report(rows):
    groups = {c: [r for r in rows if r[1] and verdict(r[1]) == c] for c in CLASSES}
    none = [r for r in rows if not r[1]]
    shipped = set(re.findall(r'alias="icons/([^"]+)\.svg"', QRC.read_text("utf-8")))
    out = ["# Icon SVGs against their PNGs", "",
           "Written by `design/tools/icon_svgs.py compare`. Each SVG (`<state>.svg` next to its PNG in",
           "`modules/gui/qt/pixmaps/redesign/icons`) is drawn by QtSvg (Qt 6.8, the renderer of the app)",
           "and compared with the PNG of the same state, in levels of 0-255:", "",
           "- **24**: drawn at the PNG's size, premultiplied RGBA pixel for pixel (max / mean).",
           "- **shape**: mean difference of the alpha averaged over 3 x 3 pixels: the shape without the",
           "  antialiasing. **colour**: largest channel difference of the mean colour where both are opaque.",
           "- **48**: drawn at 48 px against the `@2x` PNG, or (↓) averaged back to 24 against the 1x PNG.",
           "- **Figma**: mean alpha difference of the PNG with Figma's own 1x render of the symbol (the",
           "  frame 42:356), when the state shows the whole symbol; **Qt-Figma** the same for the SVG.",
           "- **offset**: the subpixel position the PNG was cut at (an instance placed between pixels),",
           "  kept in the SVG.",
           "- **app**: the app ships the SVG (`assets.qrc`): the exact and same-drawing states but the",
           "  empty ones. It keeps the PNG at 100 % and takes the SVG on a denser screen.", "",
           "No SVG drawn by QtSvg is the PNG pixel for pixel: Figma's rasteriser antialiases otherwise, and",
           "most PNGs were cut from the design's screens rather than exported from the symbol. The close",
           "states keep their PNG on every screen: their PNG is drawn heavier or lighter than Figma's own",
           "render of the symbol, so the SVG would change their weight at 150 %.", "",
           f"**{len(rows)} states: " + ", ".join(f"{len(groups[c])} {c}" for c in CLASSES)
           + f", {len(none)} without SVG; {len(shipped)} in the app.**", ""]

    def table(title, items):
        if not items:
            return []
        lines = [f"## {title} ({len(items)})", "",
                 "| State | Node | 24 max / mean | shape | colour | 48 max / mean | Figma | Qt-Figma | offset | app |",
                 "|---|---|---|---|---|---|---|---|---|---|"]
        for key, r, _ in items:
            off = "" if r["offset"] == [0, 0] else f"{fmt(r['offset'][0])}, {fmt(r['offset'][1])}"
            fig = (f"{r['figma']} | {r['qtfigma']}" if r.get("figma") is not None else " | ")
            lines.append(f"| `{key}` | {r['node'] or ''} | {r['max24']} / {r['mean24']} | {r['shape']} | {r['colour']} | "
                         f"{r['max48']} / {r['mean48']}{' ↓' if r.get('down') else ''} | {fig} | {off} | "
                         f"{'yes' if key in shipped else ''} |")
        return lines + [""]

    if none:
        out += [f"## Without SVG ({len(none)})", "", "| State | Why |", "|---|---|"]
        out += [f"| `{k}` | {why} |" for k, _, why in none] + [""]
    for c, title in CLASSES.items():
        out += table(title, groups[c])
    REPORT.write_text("\n".join(out) + "\n", "utf-8", newline="\n")
    print(f"{len(rows)} states: " + ", ".join(f"{len(groups[c])} {c}" for c in CLASSES)
          + f", {len(none)} without SVG -> {REPORT.name}")


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    sub = parser.add_subparsers(dest="command", required=True)
    for name in ("map", "extract"):
        p = sub.add_parser(name)
        p.add_argument("frame", help="SVG export of the Figma frame 42:356")
    p = sub.add_parser("compare")
    p.add_argument("--figma", help="Figma's PNG render of the frame 42:356 at 1x, without its background")
    p.add_argument("--json", help="also write the numbers to this file")
    args = parser.parse_args()
    return {"map": cmd_map, "extract": cmd_extract, "compare": cmd_compare}[args.command](args)


if __name__ == "__main__":
    sys.exit(main())
