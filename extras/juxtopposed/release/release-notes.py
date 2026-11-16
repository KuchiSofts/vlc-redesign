"""The release notes of a version from CHANGELOG.md, in the style of the
earlier releases: the Juxtopposed credit header, the section of the version,
one short table of the files of each system (those named in <dist>\\SHA256SUMS.txt)
and the unofficial footer. Written to <dist>\\release-notes-<version>.md.
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
    # (from the installer: every release has one)
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


URL = 'https://github.com/KuchiSofts/vlc-redesign/releases/download/v%s/' % version
WIKI = 'https://github.com/KuchiSofts/vlc-redesign/wiki/Install-and-Update'
V = version


def links(files):
    """Those of the files, (label, name), that are in the release: "[label](url) (size)" each"""
    return ['[%s](%s%s)%s' % (what, URL, name, ' (%s)' % size(name) if size(name) else '')
            for what, name in files if name in sums]


# One short table: a line for each system, its files as links with their size. No SHA-256 column (SHA256SUMS.txt
# has them, signed) and no line for a small update: the app takes those by itself, nobody downloads one (0.6.9's
# notes had 18 of them, each with its hash).
source = 'vlc-redesign-%s-source.tar.gz' % version
systems = [
    ('Windows 10 and 11 (64-bit)', [('Installer', 'vlc-redesign-%s-win64-setup.exe' % V),
                                    ('portable zip', 'vlc-redesign-%s-win64.zip' % V)]),
    ('Windows 11 on ARM64', [('Installer', 'vlc-redesign-%s-winarm64-setup.exe' % V),
                             ('portable zip', 'vlc-redesign-%s-winarm64.zip' % V)]),
]
for arch, a, deb, rpm in (('x86_64', 'x86_64', 'amd64', 'x86_64'), ('ARM64', 'aarch64', 'arm64', 'aarch64')):
    systems.append(('Linux %s (glibc 2.38+)' % arch,
                    [('AppImage', 'vlc-redesign-%s-%s.AppImage' % (V, a)),
                     ('folder to run from', 'vlc-redesign-%s-linux-%s.tar.xz' % (V, a)),
                     ('.deb', 'vlc-redesign_%s_%s.deb' % (V, deb)),
                     ('Live TV .deb', 'vlc-redesign-livetv_%s_%s.deb' % (V, deb)),
                     ('.rpm', 'vlc-redesign-%s-1.%s.rpm' % (V, rpm)),
                     ('Live TV .rpm', 'vlc-redesign-livetv-%s-1.%s.rpm' % (V, rpm)),
                     ('Flatpak', 'vlc-redesign-%s-%s.flatpak' % (V, a))]))
systems += [
    ('macOS 12 or later', [('Apple Silicon', 'vlc-redesign-%s-macos-arm64.dmg' % V),
                           ('Intel', 'vlc-redesign-%s-macos-x86_64.dmg' % V)]),
    ('Android 5.0 or later (phones, tablets, TV)', [('APK', 'vlc-redesign-%s-android.apk' % V)]),
    ('Samsung TV (Tizen 3.0, 2017 models, or later)', [('Juxto Media Player: source and install guide',
                                                        'Juxto-Media-Player-TV-%s-source.zip' % V)]),
    ('Source code (GPL)', [(source, source)]),
]
rows = ['| %s | %s |' % (name, ' · '.join(links(files))) for name, files in systems if links(files)]
have = {name.split(' ')[0] for name, files in systems if links(files)}

# The small update of each Windows architecture (one: from the release before; a release up to 0.6.9 had more)
small = {}
for name in sums:
    m = re.match(r'vlc-redesign-%s(-winarm64)?-delta-from-(.+)\.vrdelta$' % re.escape(V), name)
    if m:
        small.setdefault('ARM64' if m.group(1) else 'x64', []).append((m.group(2), size(name)))
update_line = '**Already have it?** The app updates itself: *About › Check for updates*.'
if small.get('x64'):
    old, mb = sorted(small['x64'])[-1]
    arm = sorted(small['ARM64'])[-1][1] if small.get('ARM64') else ''
    update_line += (' On Windows that is a small update: %s from %s%s; a copy further behind takes the ones between, '
                    'one after the other, by itself.' % (mb, old, ' (ARM64: %s)' % arm if arm else ''))

# An update release (docs/RELEASE-CHECKLIST.md: platforms.yml "full:" names an earlier version): the distribution
# packages stay those of the last full release
_p = os.path.join(ROOT, 'platforms.yml')
_full = re.search(r'^\s+full:\s*(\S+)', open(_p, encoding='utf-8').read(), re.M) if os.path.exists(_p) else None
full = _full.group(1) if _full else ''
update_note = ''
if full and full != V and 'Linux' in have:
    update_note = (' The .deb, .rpm and Flatpak packages are those of '
                   '[%s](https://github.com/KuchiSofts/vlc-redesign/releases/tag/v%s).' % (full, full))
# a paragraph of its own under the header, when written (what the release is about, in two lines)
intro_path = os.path.join(dist, 'release-intro-%s.md' % version)
intro = open(intro_path, encoding='utf-8').read().strip() + '\n\n' if os.path.exists(intro_path) else ''

# The table names every system of this release; a line more only for the Samsung TV app when it stays at an
# earlier version (platforms.yml, extras/juxtopposed/platforms/render.py)
sys.path.insert(0, os.path.join(ROOT, 'extras', 'juxtopposed', 'platforms'))
import render as platforms
platform_data = platforms.load()
samsung = next((p for p in platform_data['platforms'] if p['id'] == 'samsung-tv'), {})
also_on = ''
if 'Samsung' not in have and samsung.get('version') and samsung.get('version') != version:
    also_on = ('**Samsung TV:** *Juxto Media Player* is unchanged, at '
               '[%s](https://github.com/KuchiSofts/vlc-redesign/releases/tag/v%s).\n\n'
               % (samsung['version'], samsung['version']))
windows = next((p for p in platform_data['platforms'] if p['id'] == 'windows'), {})
if windows.get('version') != version:
    print('Note: platforms.yml names Windows %s: set its version and date to %s, then run '
          'extras/juxtopposed/platforms/render.py' % (windows.get('version'), version))

notes = '''# VLC Media Player Redesign %(v)s

> 🎨 The interface is the design of **[Juxtopposed](https://www.youtube.com/@juxtopposed)**
> ([the video](https://www.youtube.com/watch?v=ERFeOgqk324)), built on VLC 4. An unofficial build; the official VLC
> is at [videolan.org](https://www.videolan.org/vlc/).

%(intro)s%(section)s

## Downloads

| System | Files |
|---|---|
%(rows)s

%(update_line)s

**First install:** [Install and update](%(wiki)s) goes through every system step by step.%(update_note)s %(signing)s

**Checksums:** `SHA256SUMS.txt`%(sums_signed)s

%(also_on)s## Source code

VLC Redesign is free software, GPL v2 or later. The source of these files is `%(source)s` above and the tag
[`v%(v)s`](https://github.com/KuchiSofts/vlc-redesign/tree/v%(v)s); the libraries VLC uses and the written offer
for their source: [SOURCE.md](https://github.com/KuchiSofts/vlc-redesign/blob/main/docs/SOURCE.md).

---
*VLC Redesign is not affiliated with VideoLAN. VLC is a trademark of VideoLAN.*
''' % {'v': version, 'section': section, 'source': source, 'rows': '\n'.join(rows), 'also_on': also_on,
       'intro': intro, 'update_note': update_note, 'update_line': update_line, 'wiki': WIKI,
       # (the app checks it before any update: docs/code-signing.md, The release key)
       'sums_signed': (", signed by the project's release key (`SHA256SUMS.txt.sig`): the app checks it before "
                       'it takes an update.')
                      if os.path.exists(os.path.join(dist, 'SHA256SUMS.txt.sig')) else '.',
       'signing': ('The files are code-signed (certificate by SignPath Foundation, '
                   '[policy](https://github.com/KuchiSofts/vlc-redesign/blob/main/CODE_SIGNING_POLICY.md)).')
                  if signed else
                  ('The builds are not code-signed yet '
                   '([policy](https://github.com/KuchiSofts/vlc-redesign/blob/main/CODE_SIGNING_POLICY.md)): Windows '
                   'SmartScreen may ask for *More info → Run anyway*, macOS for *Open Anyway* once.')}
out = os.path.join(dist, 'release-notes-%s.md' % version)
open(out, 'w', encoding='utf-8', newline='\n').write(notes)
print(out, len(rows), 'systems', 'signed' if signed else 'not signed')
