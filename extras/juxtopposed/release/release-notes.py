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

URL = 'https://github.com/KuchiSofts/vlc-redesign/releases/download/v%s/' % version


def table(files):
    """The rows of those of the files, (label, name), that are in the release"""
    return ['| [%s](%s%s) | %s | %s | `%s` |' % (what, URL, name, name, size(name), sums[name])
            for what, name in files if name in sums]


V = version
rows = table([('Installer (Windows 10 and 11, 64-bit)', 'vlc-redesign-%s-win64-setup.exe' % V),
              ('Portable zip', 'vlc-redesign-%s-win64.zip' % V),
              ('Installer (Windows on ARM64)', 'vlc-redesign-%s-winarm64-setup.exe' % V),
              ('Portable zip (Windows on ARM64)', 'vlc-redesign-%s-winarm64.zip' % V)])
source = 'vlc-redesign-%s-source.tar.gz' % version
if source in sums:
    rows.append('| [Source code](%s%s) (GPL) | %s | %s | `%s` |' % (URL, source, source, size(source), sums[source]))
deltas = sorted(n for n in sums if n.endswith('.vrdelta'))
for name in deltas:
    old = re.search(r'delta-from-(.+)\.vrdelta$', name).group(1)
    arm = '-winarm64-delta-from-' in name
    rows.append('| Small update from %s%s (the app offers it) | %s | %s | `%s` |'
                % (old, ', ARM64' if arm else '', name, size(name), sums[name]))

# The other platforms' files: a table for each whose files are in the release
linux = []
for arch, a, deb, rpm in (('x86_64', 'x86_64', 'amd64', 'x86_64'), ('ARM64', 'aarch64', 'arm64', 'aarch64')):
    linux += table([('.deb, %s (Ubuntu 24.04+, Debian 13)' % arch, 'vlc-redesign_%s_%s.deb' % (V, deb)),
                    ('Live TV add-on .deb, %s' % arch, 'vlc-redesign-livetv_%s_%s.deb' % (V, deb)),
                    ('.rpm, %s (Fedora)' % arch, 'vlc-redesign-%s-1.%s.rpm' % (V, rpm)),
                    ('Live TV add-on .rpm, %s' % arch, 'vlc-redesign-livetv-%s-1.%s.rpm' % (V, rpm)),
                    ('AppImage, %s (any distribution with glibc 2.38+)' % arch, 'vlc-redesign-%s-%s.AppImage' % (V, a)),
                    ('Flatpak, %s (runtime from Flathub)' % arch, 'vlc-redesign-%s-%s.flatpak' % (V, a)),
                    ('Folder to run from, %s' % arch, 'vlc-redesign-%s-linux-%s.tar.xz' % (V, a))])
macos = table([('Apple Silicon (macOS 12+)', 'vlc-redesign-%s-macos-arm64.dmg' % V),
               ('Intel (macOS 12+)', 'vlc-redesign-%s-macos-x86_64.dmg' % V)])
android = table([('Phones and tablets (Android 8.0+; ARM64, ARMv7, x86, x86_64)', 'vlc-redesign-%s-android.apk' % V)])
WIKI = 'https://github.com/KuchiSofts/vlc-redesign/wiki/Install-and-Update'
HEAD = '| What | File | Size | SHA-256 |\n|---|---|---|---|\n'
others = ''
if linux:
    others += ('\n### Linux\n\n' + HEAD + '\n'.join(linux) +
               "\n\nThe .deb and .rpm install next to the distribution's own `vlc` (in `/opt/vlc-redesign`, the command "
               '`vlc-redesign`); the Live TV add-on is its own package, installed with it or left out. For example: '
               '`sudo apt install ./vlc-redesign_%s_amd64.deb ./vlc-redesign-livetv_%s_amd64.deb`. The AppImage: '
               '`chmod +x` it and run it. The Flatpak: `flatpak install --user vlc-redesign-%s-x86_64.flatpak`. To '
               "update, install the next version's file. Every way, step by step: [Install and update](%s#linux).\n"
               % (V, V, V, WIKI))
if macos:
    others += ('\n### macOS\n\n' + HEAD + '\n'.join(macos) +
               '\n\nOpen the disk image and drag VLC Redesign to Applications. It is signed ad hoc, not by Apple: the '
               'first time, macOS refuses to open it; open it once from *System Settings › Privacy & Security › Open '
               'Anyway*. Step by step: [Install and update](%s#macos).\n' % WIKI)
if android:
    others += ('\n### Android\n\n' + HEAD + '\n'.join(android) +
               '\n\nOpen the APK on the phone and allow installing from your browser or file manager once. It installs '
               '**beside** VLC from Google Play, as *VLC Media Player Redesign* (`org.kuchisofts.vlcredesign`); it is '
               'not on Google Play. Every release is signed with the same key, so a new APK installs over it as an '
               'update, and from 0.5.1 the app says when one is out. Source: '
               '[vlc-redesign-android](https://github.com/KuchiSofts/vlc-redesign-android). Step by step: '
               '[Install and update](%s#android).\n' % WIKI)
covered = ['Windows (x64 and ARM64)'] + [n for n, t in (('Linux', linux), ('macOS', macos), ('Android', android)) if t]
others += '\n`SHA256SUMS.txt` lists every file above: %s.\n' % (
    ', '.join(covered[:-1]) + ' and ' + covered[-1] if len(covered) > 1 else covered[0])
# a paragraph of its own under the header, when written (what the release is about, in two lines)
intro_path = os.path.join(dist, 'release-intro-%s.md' % version)
intro = open(intro_path, encoding='utf-8').read().strip() + '\n\n' if os.path.exists(intro_path) else ''

# "Also on": the other platforms, from platforms.yml (extras/juxtopposed/platforms/render.py)
sys.path.insert(0, os.path.join(ROOT, 'extras', 'juxtopposed', 'platforms'))
import render as platforms
platform_data = platforms.load()
also_on = platforms.also_on(platform_data, 'windows')
windows = next((p for p in platform_data['platforms'] if p['id'] == 'windows'), {})
if windows.get('version') != version:
    print('Note: platforms.yml names Windows %s: set its version and date to %s, then run '
          'extras/juxtopposed/platforms/render.py' % (windows.get('version'), version))

notes = '''# VLC Media Player Redesign %(v)s

> 🎨 The interface is the design of **[Juxtopposed](https://www.youtube.com/@juxtopposed)**
> ([the video](https://www.youtube.com/watch?v=ERFeOgqk324)), built on VLC 4. An unofficial build; the official VLC
> is at [videolan.org](https://www.videolan.org/vlc/).

%(intro)s%(section)s

## Files

### Windows

| What | File | Size | SHA-256 |
|---|---|---|---|
%(rows)s

%(signing)s A copy of 0.2.4 or later
offers the small update by itself.%(sums_signed)s
%(others)s
%(also_on)s

## Source code

VLC Redesign is free software, GPL v2 or later. The source of these files is `%(source)s` above and the tag
[`v%(v)s`](https://github.com/KuchiSofts/vlc-redesign/tree/v%(v)s); the libraries VLC uses and the written offer
for their source: [SOURCE.md](https://github.com/KuchiSofts/vlc-redesign/blob/main/docs/SOURCE.md).

---
*VLC Redesign is not affiliated with VideoLAN. VLC is a trademark of VideoLAN.*
''' % {'v': version, 'section': section, 'source': source, 'rows': '\n'.join(rows), 'also_on': also_on,
       'intro': intro, 'others': others,
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
