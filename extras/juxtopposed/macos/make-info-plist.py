#!/usr/bin/env python3
# Copyright (C) 2026 VLC authors and VideoLAN
# SPDX-License-Identifier: GPL-2.0-or-later
"""The Info.plist of VLC Redesign.app, from upstream's share/Info.plist.in.

Upstream's file is made for VLC.app and its macosx interface; this keeps
its file types and the privacy texts and changes what names the program:

- the name, the executable, the bundle id (org.kuchisofts.vlcredesign: its
  settings, library and caches are apart from those of VLC) and the icon;
- no main nib, no principal class (the Qt interface makes the application),
  no Sparkle feed of VideoLAN (it would offer VLC), no Breakpad, no accent
  colour of the macosx interface's assets;
- the type declarations of VLC are imported, not exported: VLC owns them;
- no document icons (the cone ones are VideoLAN's): Finder draws the app's
- the network schemes the browser and Finder own (http, https, ftp, sftp,
  smb) are left to them; the streaming ones stay;
- macOS 12 at least (Qt 6.8).

    make-info-plist.py --template share/Info.plist.in --version 0.4.0 \\
        --vlc-version 4.0.0-dev --executable "VLC Redesign" --out Info.plist

Python 3.8 or later, standard library only.
"""
import argparse
import datetime
import plistlib

BUNDLE_ID = 'org.kuchisofts.vlcredesign'
NAME = 'VLC Redesign'
OWNED_SCHEMES = {'http', 'https', 'ftp', 'sftp', 'smb'}


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--template', required=True)
    ap.add_argument('--version', required=True, help='the version of VLC Redesign (x.y.z)')
    ap.add_argument('--vlc-version', required=True, help='the version of VLC it is built on')
    ap.add_argument('--executable', default=NAME)
    ap.add_argument('--minimum-system', default='12.0')
    ap.add_argument('--out', required=True)
    args = ap.parse_args()

    with open(args.template, encoding='utf-8') as f:
        text = f.read()
    year = datetime.date.today().year
    text = (text.replace('@VERSION@', args.version)
                .replace('@COPYRIGHT_YEARS@', '1996-%d' % year)
                .replace('@BREAKPAD_URL@', ''))
    info = plistlib.loads(text.encode('utf-8'))

    for key in ('NSMainNibFile', 'NSPrincipalClass', 'NSAccentColorName',
                'SUFeedURL', 'SUPublicDSAKeyFile', 'SUPublicEDKey',
                'BreakpadProduct', 'BreakpadVendor', 'BreakpadRequestComments', 'BreakpadURL'):
        info.pop(key, None)

    info.update({
        'CFBundleExecutable': args.executable,
        'CFBundleIdentifier': BUNDLE_ID,
        'CFBundleName': NAME,
        'CFBundleDisplayName': NAME,
        'CFBundleIconFile': 'AppIcon.icns',
        'CFBundleSignature': '????',
        'CFBundleShortVersionString': args.version,
        'CFBundleVersion': args.version,
        'CFBundleGetInfoString': '%s %s, built on VLC %s' % (NAME, args.version, args.vlc_version),
        'LSMinimumSystemVersion': args.minimum_system,
        'NSHumanReadableCopyright':
            'Copyright (c) 1996-%d VLC authors and VideoLAN; interface design by Juxtopposed (CC BY 4.0); '
            'VLC Redesign by KuchiSofts. Not made or endorsed by VideoLAN.' % year,
    })

    for doc in info.get('CFBundleDocumentTypes', []):
        doc.pop('CFBundleTypeIconFile', None)
        # Alternate: a file opens in VLC Redesign when the user picks it,
        # not instead of the player they already use
        doc['LSHandlerRank'] = 'Alternate'

    exported = info.pop('UTExportedTypeDeclarations', [])
    imported = info.setdefault('UTImportedTypeDeclarations', [])
    known = {t.get('UTTypeIdentifier') for t in imported}
    for t in exported:
        if t.get('UTTypeIdentifier') not in known:
            imported.append(t)
    for t in imported:
        t.pop('UTTypeIconFile', None)
        t.pop('UTTypeIconFiles', None)

    schemes = []
    for url in info.get('CFBundleURLTypes', []):
        url.pop('CFBundleURLIconFile', None)
        kept = [s for s in url.get('CFBundleURLSchemes', []) if s not in OWNED_SCHEMES]
        if kept:
            url['CFBundleURLSchemes'] = kept
            schemes.append(url)
    info['CFBundleURLTypes'] = schemes

    for key, value in list(info.items()):
        if isinstance(value, str) and key.endswith('UsageDescription'):
            info[key] = value.replace('VLC wants', NAME + ' wants')

    with open(args.out, 'wb') as f:
        plistlib.dump(info, f, fmt=plistlib.FMT_XML)


if __name__ == '__main__':
    main()
