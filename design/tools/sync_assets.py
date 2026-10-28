#!/usr/bin/env python3
# Copyright (C) 2026 VLC authors and VideoLAN
# SPDX-License-Identifier: GPL-2.0-or-later
"""Copies the icons and fonts the desktop app ships into design/ and writes
design/icons/index.json and design/icons/README.md.

The app's files (modules/gui/qt/pixmaps) are the source: run this after an
icon or a font changes there. check_qml.py fails while design/ is behind.

    python design/tools/sync_assets.py           # copy and index
    python design/tools/sync_assets.py --check   # only compare (exit 1 if stale)

Standard library only.
"""

import argparse
import hashlib
import json
import re
import shutil
import struct
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
DESIGN = ROOT / "design"
PIXMAPS = ROOT / "modules" / "gui" / "qt" / "pixmaps"
REDESIGN = PIXMAPS / "redesign"

# (set name, source folder, pattern, destination folder, description)
SETS = [
    ("glyphs", REDESIGN / "icons", "*/*.png", "png",
     "The icons of the Figma \"Icon\" component set (42:356), exported as PNG, one folder per icon, "
     "one file per state. 24 x 24 unless noted; @2x files are the same state at twice the size."),
    ("svg", REDESIGN, "*.svg", "svg",
     "Single-colour SVG icons of the top bar, the sidebar and the player (24 x 24 unless noted)."),
    ("duo", REDESIGN / "duo", "*.svg", "svg/duo",
     "Two-tone menu icons. Placeholder colours (ColorizedSvgIcon): #FF00FF = outline (Dark/FG/A2 #8F8F8F, "
     "light #2E2E2E), #FF8800 = details (Accent Dim), #00FFFF = the full accent."),
    ("widgets", REDESIGN / "widgets", "*.svg", "svg/widgets",
     "Chevrons and checks of the classic (widget) dialogs, one file per theme and state."),
    ("player", REDESIGN / "player", "*.png", "player",
     "Raster player and window-frame pieces (corners per theme)."),
    ("images", REDESIGN, "*.png", "images",
     "Other raster pieces: onboarding theme previews, track icons, volume states, list/grid toggles."),
]

FONTS = ["InterVariable.ttf", "DMMono-Regular.ttf", "LICENSE.txt", "DMMono-OFL.txt"]

STATE_NOTES = {
    "default": "idle, #8F8F8F (Dark/FG/A2)",
    "active": "active or hovered, #D37918 (the default accent; the app tints it with the chosen one)",
    "selected": "selected (outline variant)",
    "dim": "dimmed, #6B6B6B or #404040",
    "disabled": "disabled, #343434",
    "default-grey": "idle, grey variant",
    "default-accent": "idle, drawn in the accent",
    "active-grey": "active, grey variant",
    "active-accent": "active, drawn in the accent",
    "outline-active": "active, outline only",
    "large": "large variant",
    "half": "half (volume, rating)",
}


def sha256(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def png_size(path):
    with path.open("rb") as f:
        head = f.read(24)
    if head[:8] != b"\x89PNG\r\n\x1a\n":
        return None
    return list(struct.unpack(">II", head[16:24]))


def svg_size(path):
    text = path.read_text(encoding="utf-8", errors="replace")[:2000]
    tag = re.search(r"<svg\b[^>]*>", text, re.S)
    if not tag:
        return None
    attrs = dict(re.findall(r'(\w[\w-]*)="([^"]*)"', tag.group(0)))

    def num(v):
        m = re.match(r"\s*([0-9.]+)", v or "")
        return float(m.group(1)) if m else None
    w, h = num(attrs.get("width")), num(attrs.get("height"))
    if (w is None or h is None) and attrs.get("viewBox"):
        parts = attrs["viewBox"].split()
        if len(parts) == 4:
            w, h = float(parts[2]), float(parts[3])
    if w is None or h is None:
        return None
    return [int(w) if w == int(w) else w, int(h) if h == int(h) else h]


def plan():
    """[(source, destination relative to design/icons, entry)]"""
    files = []
    for set_name, src_dir, pattern, dest, _ in SETS:
        for src in sorted(src_dir.glob(pattern)):
            if not src.is_file():
                continue
            if set_name == "glyphs":
                name = src.parent.name
                stem = src.stem
                scale = 2 if stem.endswith("@2x") else 1
                state = stem[:-3] if scale == 2 else stem
                rel = f"{dest}/{name}/{src.name}"
            else:
                name = src.stem
                scale = 1
                state = "default"
                rel = f"{dest}/{src.name}"
            size = png_size(src) if src.suffix == ".png" else svg_size(src)
            files.append((src, rel, {
                "set": set_name, "name": name, "state": state, "scale": scale,
                "format": src.suffix[1:], "file": rel, "size": size,
                "source": src.relative_to(ROOT).as_posix(), "sha256": sha256(src),
            }))
    return files


def build_index(files):
    icons = {}
    for _, _, e in files:
        key = (e["set"], e["name"])
        icon = icons.setdefault(key, {"set": e["set"], "name": e["name"], "states": []})
        icon["states"].append({k: e[k] for k in ("state", "scale", "format", "file", "size", "source", "sha256")})
    return {
        "$description": "The icon set of the VLC Redesign (design-1.0): every icon the desktop app ships, "
                        "copied from modules/gui/qt/pixmaps/redesign by design/tools/sync_assets.py.",
        "version": "design-1.0",
        "license": "CC BY 4.0, derived from \"VLC Redesign (Community)\" by Juxtopposed",
        "sets": {s[0]: {"folder": s[3], "description": s[4]} for s in SETS},
        "states": STATE_NOTES,
        "icons": [icons[k] for k in sorted(icons, key=lambda k: ([s[0] for s in SETS].index(k[0]), k[1].lower()))],
    }


def build_readme(index):
    out = ["# Icons", "",
           "The icon set of the VLC Redesign, `design-1.0`. Generated by `design/tools/sync_assets.py` from",
           "`modules/gui/qt/pixmaps/redesign`; do not edit by hand. `index.json` lists every file with its",
           "name, state, scale, size and checksum.", "",
           "Derived from *VLC Redesign (Community)* by Juxtopposed, CC BY 4.0 (see `design/README.md`).", "",
           "**SVG sources.** The glyphs of the Figma *Icon* set (42:356) exist only as the PNG files exported",
           "from Figma (`png/`, with `@2x` where the app needs it); the repository has no SVG source for them.",
           "The top bar, sidebar and player icons (`svg/`), the two-tone menu icons (`svg/duo/`) and the",
           "widget chevrons (`svg/widgets/`) are SVG.", "",
           "**States.** " + "; ".join(f"`{k}`: {v}" for k, v in STATE_NOTES.items()) + ".", ""]
    by_set = {}
    for icon in index["icons"]:
        by_set.setdefault(icon["set"], []).append(icon)
    for set_name, info in index["sets"].items():
        icons = by_set.get(set_name, [])
        count = sum(len(i["states"]) for i in icons)
        out += [f"## {set_name} (`{info['folder']}/`, {len(icons)} icons, {count} files)", "", info["description"], ""]
        out += ["| Icon | States | Size |", "|---|---|---|"]
        for icon in icons:
            cells = []
            sizes = set()
            for s in icon["states"]:
                label = s["state"] + ("@2x" if s["scale"] == 2 else "")
                cells.append(f'<img src="{s["file"]}" height="24" alt="{icon["name"]} {label}"> `{label}`')
                if s["size"]:
                    sizes.add(" x ".join(str(v) for v in s["size"]))
            out.append(f"| `{icon['name']}` | {' '.join(cells)} | {', '.join(sorted(sizes))} |")
        out.append("")
    return "\n".join(out)


def expected():
    """{path relative to design/: bytes}"""
    files = plan()
    result = {}
    for src, rel, _ in files:
        result[f"icons/{rel}"] = src.read_bytes()
    index = build_index(files)
    result["icons/index.json"] = (json.dumps(index, indent=2, ensure_ascii=False) + "\n").encode("utf-8")
    result["icons/README.md"] = (build_readme(index) + "\n").encode("utf-8")
    for name in FONTS:
        result[f"fonts/{name}"] = (PIXMAPS / "fonts" / name).read_bytes()
    return result


TEXT = (".json", ".md", ".svg", ".txt")


def unify(data, rel):
    """Text files compare without their line endings (a CRLF checkout)"""
    return data.replace(b"\r\n", b"\n") if rel.endswith(TEXT) else data


def compare():
    """Problems between design/icons, design/fonts and the app's files"""
    want = expected()
    problems = []
    for rel, data in want.items():
        path = DESIGN / rel
        if not path.exists():
            problems.append(f"missing: design/{rel}")
        elif unify(path.read_bytes(), rel) != unify(data, rel):
            problems.append(f"differs from the app's file: design/{rel}")
    managed = set(want)
    for folder in ("icons",):
        for path in (DESIGN / folder).rglob("*"):
            rel = path.relative_to(DESIGN).as_posix()
            if path.is_file() and rel not in managed:
                problems.append(f"not in the app any more: design/{rel}")
    return problems


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--check", action="store_true", help="only compare, exit 1 if design/ is stale")
    args = parser.parse_args()
    if args.check:
        problems = compare()
        for p in problems:
            print(p)
        print("icons and fonts: " + ("up to date" if not problems else f"{len(problems)} problem(s)"))
        return 1 if problems else 0
    want = expected()
    icons_dir = DESIGN / "icons"
    if icons_dir.exists():
        shutil.rmtree(icons_dir)
    for rel, data in want.items():
        path = DESIGN / rel
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_bytes(data)
    print(f"wrote {len(want)} files under design/")
    return 0


if __name__ == "__main__":
    sys.exit(main())
