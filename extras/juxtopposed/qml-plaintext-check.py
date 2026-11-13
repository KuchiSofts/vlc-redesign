#!/usr/bin/env python3
# Copyright (C) 2026 VLC authors and VideoLAN
# SPDX-License-Identifier: GPL-2.0-or-later
"""Fails when a QML Text shows outside data (a media title, a channel name, a device name...) in the default format.

A Text with no textFormat is AutoText: a string that looks like HTML is drawn as HTML, and an <img> in it is loaded
from its address. So a playlist, a guide or a device on the network could change what the interface shows and make
the app ask a server of theirs. Every Text whose text: is not a fixed string needs a textFormat (Text.PlainText,
or Text.StyledText where the QML builds the HTML itself and escapes what it puts in).

    python3 extras/juxtopposed/qml-plaintext-check.py      # exit 1 on any such Text
"""
import os
import re
import sys

ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', 'modules', 'gui', 'qt')
# a text: that names one of these shows data from outside the app
OUTSIDE = re.compile(r'\b(model\.|modelData|title|name|artist|album|programme|program|channel|description|subtitle|'
                     r'display|label|mrl|url|lang|genre|showName|show|provider|source|summary|text)\w*', re.I)


def blocks(src):
    """(line, own lines) of every Text { ... } (its children's lines left out)"""
    for m in re.finditer(r'(^|[\s:])(Text|T\.Label|Controls\.Label|Label)\s*\{', src):
        depth, i = 1, m.end()
        while i < len(src) and depth:
            if src[i] == '{':
                depth += 1
            elif src[i] == '}':
                depth -= 1
            i += 1
        own = src[m.end():i]
        while True:
            inner = re.sub(r'\{[^{}]*\}', '', own)
            if inner == own:
                break
            own = inner
        yield src.count('\n', 0, m.start()) + 1, own


TEXT_ROOT = re.compile(r'^\s*(Text|T\.Label|Controls\.Label|Label|T\.TextArea|TextEdit)\s*\{', re.M)
# components whose root is a Text and that leave textFormat to their users, with the reason
ROOT_ALLOWED = {
    'LabelExt.qml': 'the update pane colours the version with <font>',
    'RedesignSectionTitle.qml': "the app's own section names only",
    'EditorDummyButton.qml': "the toolbar editor's own button names",
    'RedesignTextArea.qml': 'a text box: what is typed is never parsed (TextArea is plain by default)',
}


def root_type(src):
    """the type of the file's root object (after the imports), or None"""
    body = re.sub(r'//[^\n]*|/\*.*?\*/', '', src, flags=re.S)
    last = body.rfind('\nimport ')
    m = TEXT_ROOT.search(body[body.find('\n', last + 1) if last >= 0 else 0:])
    first = re.search(r'^\s*[A-Z][\w.]*\s*\{', body[body.find('\n', last + 1) if last >= 0 else 0:], re.M)
    return m.group(1) if m and first and m.start() == first.start() else None


def main():
    found = []
    for folder, _, names in os.walk(ROOT):
        for name in sorted(names):
            if not name.endswith('.qml'):
                continue
            path = os.path.join(folder, name)
            with open(path, encoding='utf-8') as f:
                src = f.read()
            # a component that is a Text (the player's title, a label): its users set its text, so it says
            # how the text is drawn itself
            kind = root_type(src)
            if kind and name not in ROOT_ALLOWED and not re.search(r'(?m)^ {4}textFormat:', src):
                found.append('%s  a %s component with no textFormat of its own' % (os.path.relpath(path, ROOT), kind))
            for line, own in blocks(src):
                tm = re.search(r'(?m)^\s*text:\s*(.+)$', own)
                if not tm or 'textFormat' in own:
                    continue
                expr = tm.group(1).strip()
                if expr.startswith(('qsTr(', '"', "'", 'I18n')) and '+' not in expr and '.arg(' not in expr:
                    continue
                if OUTSIDE.search(expr):
                    found.append('%s:%d  text: %s' % (os.path.relpath(path, ROOT), line, expr[:90]))
    for f in found:
        print(f)
    if found:
        print('%d Text item(s) show outside data with the default (rich) format: add textFormat: Text.PlainText'
              % len(found))
        return 1
    print('qml-plaintext-check: every Text that shows outside data has a textFormat')
    return 0


if __name__ == '__main__':
    sys.exit(main())
