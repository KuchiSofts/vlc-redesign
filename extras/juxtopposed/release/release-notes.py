"""The release notes of a version from CHANGELOG.md, in the style of the
earlier releases: the Juxtopposed credit header, the section of the version,
the table of files with their SHA-256 (from <dist>\\SHA256SUMS.txt) and the
unofficial footer. Written to <dist>\\release-notes-<version>.md.
  python release-notes.py <version> [<dist>] [--signed | --unsigned]
Whether the files are code-signed is read from the installer (its
Authenticode signature), unless --signed or --unsigned says it."""
import os, re, struct, sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))
version = sys.argv[1]
args = [a for a in sys.argv[2:] if a not in ('--signed', '--unsigned')]
dist = args[0] if args else os.path.join(os.path.expanduser('~'), 'source', 'vlc-redesign-dist')


def pe_signed(path):
    """Whether a PE file carries a signature (a certificate table)"""
    try:
        data = open(path, 'rb').read(4096)
        pe = struct.unpack_from('<I', data, 0x3C)[0]
        if data[:2] != b'MZ' or data[pe:pe + 4] != b'PE\0\0':
            return False
        optional = pe + 24
        magic = struct.unpack_from('<H', data, optional)[0]
        # the security directory, the 5th of the data directories
        directories = optional + (112 if magic == 0x20B else 96)
        address, length = struct.unpack_from('<II', data, directories + 4 * 8)
        return address != 0 and length != 0
    except (OSError, struct.error):
        return False


if '--signed' in sys.argv:
    signed = True
elif '--unsigned' in sys.argv:
    signed = False
else:
    signed = pe_signed(os.path.join(dist, 'vlc-redesign-%s-win64-setup.exe' % version))

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

# "Also on": the other platforms, from platforms.yml (extras/juxtopposed/platforms/render.py)
sys.path.insert(0, os.path.join(ROOT, 'extras', 'juxtopposed', 'platforms'))
import render as platforms
platform_data = platforms.load()
also_on = platforms.also_on(platform_data, 'windows')
windows = next((p for p in platform_data['platforms'] if p['id'] == 'windows'), {})
if windows.get('version') != version:
    print('Note: platforms.yml names Windows %s: set its version and date to %s, then run '
          'extras/juxtopposed/platforms/render.py' % (windows.get('version'), version))

notes = '''# VLC Redesign %(v)s

> 🎨 The interface is the design of **[Juxtopposed](https://www.youtube.com/@juxtopposed)**
> ([the video](https://www.youtube.com/watch?v=ERFeOgqk324)), built on VLC 4. An unofficial build; the official VLC
> is at [videolan.org](https://www.videolan.org/vlc/).

%(section)s

## Files

| What | File | Size | SHA-256 |
|---|---|---|---|
%(rows)s

%(signing)s A copy of 0.2.4 or later
offers the small update by itself.%(sums_signed)s

%(also_on)s

---
*VLC Redesign is not affiliated with VideoLAN. VLC is a trademark of VideoLAN.*
''' % {'v': version, 'section': section, 'rows': '\n'.join(rows), 'also_on': also_on,
       # (the app checks it before any update: docs/code-signing.md, The release key)
       'sums_signed': (' `SHA256SUMS.txt.sig` is the signature of the checksums by the project\'s release\n'
                       'key, which the app checks before it installs an update.')
                      if os.path.exists(os.path.join(dist, 'SHA256SUMS.txt.sig')) else '',
       'signing': ('The files are code-signed (certificate by SignPath Foundation,\n'
                   '[policy](https://github.com/KuchiSofts/vlc-redesign/blob/main/CODE_SIGNING_POLICY.md)).')
                  if signed else
                  ('Windows SmartScreen may ask for *More info → Run anyway*: the builds are not code-signed yet\n'
                   '([policy](https://github.com/KuchiSofts/vlc-redesign/blob/main/CODE_SIGNING_POLICY.md)).')}
out = os.path.join(dist, 'release-notes-%s.md' % version)
open(out, 'w', encoding='utf-8', newline='\n').write(notes)
print(out, len(rows), 'files', 'signed' if signed else 'not signed')
