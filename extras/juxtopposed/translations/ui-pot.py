#!/usr/bin/env python3
"""The texts of the Qt Designer forms (modules/gui/qt/**/*.ui) as a POT file.

uic turns each <string> of a form into qtr("...") (-tr qtr), so VLC looks
these texts up in its catalogs, but xgettext cannot read a .ui file:
VideoLAN's po/vlc.pot gets them from the ui_*.h of a build (POTFILES.in,
"qt .ui files"). Without them, a po/vlc.pot made from the source tree alone
loses every text of the classic windows (Open Media's "Capture mode",
"Device Selection", "Show more options"...), msgmerge makes their
translations obsolete, and the windows show English in every language.

  ui-pot.py OUT.pot      then: msgcat --use-first -o vlc.pot xgettext.pot OUT.pot

The references name the .ui file and the line of the <string>.
"""
import os
import re
import subprocess
import sys
import xml.etree.ElementTree as ET

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..', '..'))
BS = chr(92)
# properties uic never passes to qtr()
SKIP = {'objectName', 'styleSheet', 'buddy', 'class', 'name'}


def escape(s):
    return (s.replace(BS, BS + BS).replace('"', BS + '"')
             .replace('\t', BS + 't').replace('\r', '').replace('\n', BS + 'n'))


def strings(path):
    """(line, text) of the translatable <string> elements of a form."""
    with open(path, encoding='utf-8') as f:
        source = f.read()
    # the line of each <string ...> opening, in document order
    lines = [source.count('\n', 0, m.start()) + 1 for m in re.finditer(r'<string\b', source)]
    root = ET.fromstring(source)
    parents = {child: parent for parent in root.iter() for child in parent}
    out = []
    for i, node in enumerate(root.iter('string')):
        text = node.text or ''
        owner = parents.get(node)
        name = owner.get('name') if owner is not None else None
        if node.get('notr') == 'true' or not text.strip() or name in SKIP:
            continue
        out.append((lines[i], text))
    return out


def main():
    if len(sys.argv) != 2:
        sys.exit(__doc__)
    files = subprocess.run(['git', '-C', ROOT, 'ls-files', 'modules/gui/qt/*.ui'],
                           capture_output=True, text=True, check=True).stdout.split()
    entries = {}
    for rel in sorted(files):
        for line, text in strings(os.path.join(ROOT, rel)):
            entries.setdefault(text, []).append('%s:%d' % (rel, line))
    with open(sys.argv[1], 'w', encoding='utf-8', newline='\n') as f:
        f.write('msgid ""\nmsgstr ""\n"Content-Type: text/plain; charset=UTF-8\\n"\n')
        for text, refs in entries.items():
            f.write('\n#: %s\n' % ' '.join(refs))
            f.write('msgid "%s"\nmsgstr ""\n' % escape(text))
    print('%d texts from %d forms' % (len(entries), len(files)))


if __name__ == '__main__':
    main()
