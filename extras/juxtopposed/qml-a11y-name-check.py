#!/usr/bin/env python3
# Copyright (C) 2026 VLC authors and VideoLAN
# SPDX-License-Identifier: GPL-2.0-or-later
"""Fails when a QML text field or settings toggle has no accessible name.

A placeholder is not a name: Narrator and other screen readers read such a field as an unnamed "edit", and a
toggle as a bare "check box". Every TextInput, TextField, T.TextField, TextArea and SettingsToggle needs an
Accessible.name (or Accessible.ignored), unless it is the root of a component that its users name.

    python3 extras/juxtopposed/qml-a11y-name-check.py      # exit 1 on any unnamed one
"""
import os
import re
import sys

ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', 'modules', 'gui', 'qt')
TYPES = r'TextInput|TextField|T\.TextField|Controls\.TextField|TextArea|T\.TextArea|SettingsToggle'
# components (named where they are used) whose own body is the field
COMPONENTS = re.compile(r'(^|\n)\s*component\s+\w+\s*:\s*$')


def blocks(src):
    """(line, body, is the root or a component's root) of every field"""
    for m in re.finditer(r'(?<![\w.])(' + TYPES + r')\s*\{', src):
        depth, i = 1, m.end()
        while i < len(src) and depth:
            if src[i] == '{':
                depth += 1
            elif src[i] == '}':
                depth -= 1
            i += 1
        before = src[:m.start()]
        # the file's root (only imports, comments and pragmas before it) or `component X: TextField {`
        stripped = re.sub(r'//[^\n]*|/\*.*?\*/|^\s*(import|pragma)[^\n]*', '', before, flags=re.S | re.M).strip()
        is_root = not stripped or bool(COMPONENTS.search(before + '\n')) \
            or bool(re.search(r'component\s+\w+\s*:\s*$', before))
        yield src.count('\n', 0, m.start()) + 1, src[m.end():i], is_root


def main():
    found = []
    for folder, _, names in os.walk(ROOT):
        for name in sorted(names):
            if not name.endswith('.qml'):
                continue
            path = os.path.join(folder, name)
            with open(path, encoding='utf-8') as f:
                src = f.read()
            for line, body, is_root in blocks(src):
                if is_root or 'Accessible.name' in body or 'Accessible.ignored' in body:
                    continue
                found.append('%s:%d' % (os.path.relpath(path, ROOT), line))
    for f in found:
        print(f)
    if found:
        print('%d field(s) or toggle(s) with no Accessible.name: a screen reader reads them unnamed' % len(found))
        return 1
    print('qml-a11y-name-check: every text field and settings toggle has an accessible name')
    return 0


if __name__ == '__main__':
    sys.exit(main())
