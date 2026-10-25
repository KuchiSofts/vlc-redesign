"""The release notes of a version from CHANGELOG.md, in the style of the
earlier releases: the Juxtopposed credit header, the section of the version,
the table of files with their SHA-256 (from <dist>\\SHA256SUMS.txt) and the
unofficial footer. Written to <dist>\\release-notes-<version>.md.
  python release-notes.py <version> [<dist>]"""
import os, re, sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))
version = sys.argv[1]
dist = sys.argv[2] if len(sys.argv) > 2 else os.path.join(os.path.expanduser('~'), 'source', 'vlc-redesign-dist')

changelog = open(os.path.join(ROOT, 'CHANGELOG.md'), encoding='utf-8').read()
m = re.search(r'^## \[%s\][^\n]*\n(.*?)(?=^## \[|\Z)' % re.escape(version), changelog, re.S | re.M)
if not m:
    sys.exit('CHANGELOG.md has no section for %s' % version)
section = m.group(1).strip()
# the anchors of the changelog point into the file: into the repository here
section = re.sub(r'\]\(#', '](https://github.com/KuchiSofts/vlc-redesign/blob/main/CHANGELOG.md#', section)

sums = {}
path = os.path.join(dist, 'SHA256SUMS.txt')
if os.path.exists(path):
    for line in open(path, encoding='utf-8'):
        parts = line.split()
        if len(parts) == 2:
            sums[parts[1]] = parts[0]

def size(name):
    p = os.path.join(dist, name)
    return '%.1f MB' % (os.path.getsize(p) / 1048576) if os.path.exists(p) else ''

rows = []
for name, what in [('vlc-redesign-%s-win64-setup.exe' % version, 'Installer (Windows 10 and 11, 64-bit)'),
                   ('vlc-redesign-%s-win64.zip' % version, 'Portable zip')]:
    if name in sums:
        rows.append('| [%s](https://github.com/KuchiSofts/vlc-redesign/releases/download/v%s/%s) | %s | %s | `%s` |'
                    % (what, version, name, name, size(name), sums[name]))
deltas = sorted(n for n in sums if n.endswith('.vrdelta'))
for name in deltas:
    old = re.search(r'delta-from-(.+)\.vrdelta$', name).group(1)
    rows.append('| Small update from %s (the app offers it) | %s | %s | `%s` |' % (old, name, size(name), sums[name]))

notes = '''# VLC Redesign %(v)s

> 🎨 The interface is the design of **[Juxtopposed](https://www.youtube.com/@juxtopposed)**
> ([the video](https://www.youtube.com/watch?v=ERFeOgqk324)), built on VLC 4. An unofficial build; the official VLC
> is at [videolan.org](https://www.videolan.org/vlc/).

%(section)s

## Files

| What | File | Size | SHA-256 |
|---|---|---|---|
%(rows)s

Windows SmartScreen may ask for *More info → Run anyway*: the builds are not code-signed yet
([policy](https://github.com/KuchiSofts/vlc-redesign/blob/main/CODE_SIGNING_POLICY.md)). A copy of 0.2.4 or later
offers the small update by itself.

---
*VLC Redesign is not affiliated with VideoLAN. VLC is a trademark of VideoLAN.*
''' % {'v': version, 'section': section, 'rows': '\n'.join(rows)}
out = os.path.join(dist, 'release-notes-%s.md' % version)
open(out, 'w', encoding='utf-8', newline='\n').write(notes)
print(out, len(rows), 'files')
