#!/usr/bin/env python3
# Copyright (C) 2026 VLC authors and VideoLAN
# SPDX-License-Identifier: GPL-2.0-or-later
"""Checks that the desktop app uses the values of design/tokens.json.

For every token, $extensions["org.vlc-redesign"].code lists where the code
writes the value:

    {"file": "...", "find": "readonly property color textMuted: \"{value}\""}

"find" is literal text with {value} where the value stands (any run of
spaces or line breaks matches any other); "after" starts the search after a
literal anchor; "nth" picks one match (0 = the first), otherwise every match
must hold the value. For a shadow, "field" names the part (offsetY, blur,
spread, alpha) and "factor" what the code multiplies it by.

It also checks:
  - VLCStyle.redesign.accentChoices against accent.choice (order and colours),
  - Dim, Bright and Bright (light) of every accent against the rule
    (0.81 x, 0.26 x, #DEDEDE tinted 35 %; 1 per channel of rounding allowed),
  - the light colour map of VLCStyle.redesign.color() against the light
    tokens, both ways (a light token whose dark value the map turns into
    something else fails: the map wins over a fallback),
  - design/icons and design/fonts against the app's files (sync_assets.py),
  - design/build against what export.py generates now.

    python design/tools/check_qml.py        # exit 1 on any difference

Standard library only.
"""

import json
import re
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[1]
TOKENS = ROOT / "design" / "tokens.json"
VLCSTYLE = ROOT / "modules" / "gui" / "qt" / "style" / "VLCStyle.qml"
EXT = "org.vlc-redesign"

sys.path.insert(0, str(HERE))
import export  # noqa: E402
import sync_assets  # noqa: E402

COLOR_RE = r"(#[0-9A-Fa-f]{8}|#[0-9A-Fa-f]{6}|0x[0-9A-Fa-f]{2},\s*0x[0-9A-Fa-f]{2},\s*0x[0-9A-Fa-f]{2})"
NUMBER_RE = r"(-?(?:[0-9]+(?:\.[0-9]*)?|\.[0-9]+))"


def walk(node, path=(), inherited=None):
    """Yields (path, token, $type) for every token"""
    if not isinstance(node, dict):
        return
    kind = node.get("$type", inherited)
    if "$value" in node:
        yield path, node, kind
        return
    for key, child in node.items():
        if not key.startswith("$"):
            yield from walk(child, path + (key,), kind)


def norm_color(text):
    text = text.strip()
    if text.startswith("0x"):
        return "#" + "".join(p.strip()[2:] for p in text.split(",")).upper()
    return text.upper()


def number(value):
    if isinstance(value, (int, float)):
        return float(value)
    return float(re.match(r"-?[0-9.]+", str(value)).group(0))


def literal(text):
    """Regex for literal text, any whitespace run matching any other"""
    parts = re.split(r"(\s+)", text)
    return "".join(r"\s+" if p.isspace() else re.escape(p) for p in parts if p)


def pattern(find, kind):
    if "{value}" not in find:
        raise ValueError(f"no {{value}} in {find!r}")
    before, after = find.split("{value}", 1)
    return re.compile(literal(before) + (COLOR_RE if kind == "color" else NUMBER_RE) + literal(after))


_files = {}


def source(rel):
    if rel not in _files:
        _files[rel] = (ROOT / rel).read_text(encoding="utf-8", errors="replace")
    return _files[rel]


def expected_value(token, kind, ref):
    """(value to compare, kind of comparison) for a code reference"""
    value = token["$value"]
    if kind == "shadow":
        field = ref["field"]
        if field == "alpha":
            color = value["color"]
            return int(color[7:9], 16) if len(color) == 9 else 255, "alpha"
        return number(value[field]) * ref.get("factor", 1), "number"
    if kind == "color":
        return value.upper(), "color"
    return number(value) * ref.get("factor", 1), "number"


def same(want, got, how):
    if how == "color":
        return norm_color(got) == want
    if how == "alpha":
        return round(float(got) * 255) == want
    return abs(float(got) - want) < 1e-6


def check_ref(name, token, kind, ref):
    rel = ref["file"]
    try:
        text = source(rel)
    except OSError:
        return [f"{name}: {rel} not found"]
    start = 0
    if "after" in ref:
        m = re.search(literal(ref["after"]), text)
        if not m:
            return [f"{name}: {rel}: anchor {ref['after']!r} not found"]
        start = m.end()
    regex = pattern(ref["find"], "color" if kind == "color" or ref.get("field") == "color" else "number")
    matches = list(regex.finditer(text, start))
    if "nth" in ref:
        if len(matches) <= ref["nth"]:
            return [f"{name}: {rel}: {ref['find']!r} not found (match {ref['nth']} of {len(matches)})"]
        matches = [matches[ref["nth"]]]
    if not matches:
        return [f"{name}: {rel}: {ref['find']!r} not found"]
    want, how = expected_value(token, kind, ref)
    problems = []
    for m in matches:
        if not same(want, m.group(1), how):
            line = text.count("\n", 0, m.start()) + 1
            shown = f"#{want:02X} alpha" if how == "alpha" else want
            problems.append(f"{name}: {rel}:{line} has {m.group(1)}, tokens.json {shown}")
    return problems


def get(tokens, dotted):
    node = tokens
    for key in dotted.split("."):
        node = node[key]
    return node


def rgb(hex_color):
    return [int(hex_color[i:i + 2], 16) for i in (1, 3, 5)]


def check_accents(tokens, style):
    problems = []
    choices = tokens["accent"]["choice"]
    names = [k for k in choices if not k.startswith("$")]
    m = re.search(r"accentChoices:\s*\[([^\]]*)\]", style)
    code = [c.upper() for c in re.findall(r"#[0-9A-Fa-f]{6}", m.group(1))] if m else []
    mine = [choices[n]["main"]["$value"].upper() for n in names]
    if code != mine:
        problems.append(f"accent.choice: VLCStyle accentChoices {code}, tokens.json {mine}")
    rule = tokens["accent"]["rule"]
    dim_f, bright_f = rule["dimFactor"]["$value"], rule["brightFactor"]["$value"]
    base, amount = rgb(rule["lightTintBase"]["$value"]), rule["lightTintAmount"]["$value"]
    default = tokens["accent"]["default"]["$value"].upper()
    for n in names:
        main = choices[n]["main"]["$value"].upper()
        c = rgb(main)
        if main == default:
            want_dim = rgb(tokens["accent"]["defaultDim"]["$value"])
            want_bright = rgb(tokens["accent"]["defaultBright"]["$value"])
        else:
            want_dim = [v * dim_f for v in c]
            want_bright = [v * bright_f for v in c]
        want_light = [b + (v - b) * amount for b, v in zip(base, c)]
        for part, want in (("dim", want_dim), ("bright", want_bright), ("brightLight", want_light)):
            got = rgb(choices[n][part]["$value"])
            if any(abs(g - w) > 1.0 for g, w in zip(got, want)):
                problems.append(f"accent.choice.{n}.{part}: {choices[n][part]['$value']}, the rule gives "
                                + "#" + "".join(f"{round(w):02X}" for w in want))
    return problems


def check_light_map(tokens, style):
    problems = []
    m = re.search(r"_lightColors:\s*\(\{(.*?)\}\)", style, re.S)
    if not m:
        return ["VLCStyle.qml: _lightColors not found"]
    table = {k.upper(): v.upper() for k, v in re.findall(r'"(#[0-9A-Fa-f]{6})":\s*"(#[0-9A-Fa-f]{6})"', m.group(1))}
    dark = {}
    for path, token, kind in walk(tokens["color"]["dark"], ("color", "dark")):
        dark.setdefault(token["$value"].upper(), []).append(".".join(path))
    pairs = {}
    for path, token, kind in walk(tokens["color"]["light"], ("color", "light")):
        link = token.get("$extensions", {}).get(EXT, {}).get("darkToken")
        if link:
            pairs[link] = (".".join(path), token["$value"].upper())
    for path_dark, (path_light, light) in pairs.items():
        value = get(tokens, path_dark)["$value"].upper()
        if value in table and table[value] != light:
            problems.append(f"{path_light}: {light}, but VLCStyle.redesign.color({value}) gives {table[value]}")
    for key, light in table.items():
        names = dark.get(key)
        if not names:
            problems.append(f"VLCStyle _lightColors {key} -> {light}: no dark token has {key}")
            continue
        if not any(n in pairs and pairs[n][1] == light for n in names):
            problems.append(f"VLCStyle _lightColors {key} -> {light}: no light token of {', '.join(names)} has {light}")
    return problems


def check_tokens(tokens):
    problems, checked, unchecked, refs = [], 0, [], 0
    for path, token, kind in walk(tokens):
        name = ".".join(path)
        if "$description" not in token:
            problems.append(f"{name}: no $description")
        code = token.get("$extensions", {}).get(EXT, {}).get("code", [])
        if not code:
            unchecked.append(name)
            continue
        checked += 1
        for ref in code:
            refs += 1
            problems += check_ref(name, token, kind, ref)
    return problems, checked, unchecked, refs


def main():
    tokens = json.loads(TOKENS.read_text(encoding="utf-8"))
    style = VLCSTYLE.read_text(encoding="utf-8")
    problems, checked, unchecked, refs = check_tokens(tokens)
    accent = check_accents(tokens, style)
    light = check_light_map(tokens, style)
    assets = sync_assets.compare()
    build = export.compare()
    for title, items in (("tokens and code", problems), ("accents", accent), ("light colours", light),
                         ("icons and fonts (run design/tools/sync_assets.py)", assets),
                         ("design/build (run design/tools/export.py)", build)):
        print(f"{title}: {'ok' if not items else f'{len(items)} difference(s)'}")
        for p in items:
            print(f"  {p}")
    print(f"{checked} tokens checked against {refs} places in the code; "
          f"{len(unchecked)} design-only tokens (no single place in the code holds them)")
    return 1 if problems or accent or light or assets or build else 0


if __name__ == "__main__":
    sys.exit(main())
