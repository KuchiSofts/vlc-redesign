#!/usr/bin/env python3
# Copyright (C) 2026 VLC authors and VideoLAN
# SPDX-License-Identifier: GPL-2.0-or-later
#
# Static check of a Windows package (win64-docker.ps1 / win64-docker.sh),
# x64 or ARM64, that runs anywhere (the Linux build job, any PC):
#   1. every .exe and .dll has the package's PE machine (ARM64: 0xAA64, x64:
#      0x8664): no stray x64 or x86 file in an ARM64 package (Windows would
#      run it emulated, or a plugin would not load into an ARM64 vlc.exe);
#   2. every imported DLL (plain and delay-loaded) is found the way Windows
#      finds it for that file: next to it, in the package's folder (where
#      vlc.exe and libvlccore.dll are; plugins load with that folder in the
#      search), or in Windows itself (System32, the API sets);
#   3. the parts every copy needs: the Qt interface (Qt and its QML built
#      in: no Qt DLL ships), the Live TV add-on, the translations, the Lua
#      scripts, the texts (credits naming the designer).
# Needs pefile (pip install pefile). The exit code is the number of
# failures.
#
#   extras/juxtopposed/ci/pe-check.py <package folder> [--arch arm64|x64]

import argparse
import os
import sys

try:
    import pefile
except ImportError:
    sys.exit('pe-check: needs pefile (pip install pefile)')

MACHINES = {0xAA64: 'ARM64', 0x8664: 'x64', 0x14C: 'x86', 0x1C4: 'ARM', 0xA641: 'ARM64EC'}

# The DLLs of Windows (10 1809 or later, x64 and ARM64) the package may
# import; anything else must ship with it. Kept short on purpose: a DLL that
# appears here for the first time is looked at before it is added.
SYSTEM_DLLS = {
    'advapi32.dll', 'avrt.dll', 'bcrypt.dll', 'cfgmgr32.dll', 'combase.dll', 'comctl32.dll', 'comdlg32.dll',
    'crypt32.dll', 'd2d1.dll', 'd3d11.dll', 'd3d12.dll', 'd3d9.dll', 'd3dcompiler_47.dll', 'dbghelp.dll',
    'dcomp.dll', 'dnsapi.dll', 'dwmapi.dll', 'dwrite.dll', 'dxgi.dll', 'dxva2.dll', 'evr.dll', 'gdi32.dll',
    'gdiplus.dll', 'hid.dll', 'imm32.dll', 'iphlpapi.dll', 'kernel32.dll', 'ksuser.dll', 'mf.dll',
    'mfplat.dll', 'mfreadwrite.dll', 'mfuuid.dll', 'mmdevapi.dll', 'mpr.dll', 'msimg32.dll', 'msvcrt.dll',
    'ncrypt.dll', 'netapi32.dll', 'normaliz.dll', 'ntdll.dll', 'ole32.dll', 'oleacc.dll', 'oleaut32.dll',
    'opengl32.dll', 'powrprof.dll', 'propsys.dll', 'psapi.dll', 'rpcrt4.dll', 'secur32.dll', 'setupapi.dll',
    'shcore.dll', 'shell32.dll', 'shlwapi.dll', 'srvcli.dll', 'urlmon.dll', 'user32.dll', 'userenv.dll',
    'uxtheme.dll', 'version.dll', 'winhttp.dll', 'wininet.dll', 'winmm.dll', 'winspool.drv', 'wintrust.dll',
    'wldap32.dll', 'wlanapi.dll', 'ws2_32.dll', 'wsock32.dll', 'wtsapi32.dll', 'xinput1_4.dll',
    'xinput9_1_0.dll', 'authz.dll', 'credui.dll', 'dhcpcsvc.dll', 'mswsock.dll', 'sspicli.dll',
    'virtdisk.dll', 'wevtapi.dll', 'windowscodecs.dll', 'mfcore.dll', 'd3d10.dll', 'd3d10_1.dll',
    'dsound.dll', 'msacm32.dll', 'mmdevapi.dll', 'runtimeobject.dll', 'api-ms-win-core-synch-l1-2-0.dll',
    'bluetoothapis.dll', 'cryptbase.dll', 'ucrtbase.dll', 'dxcore.dll', 'icu.dll', 'msdmo.dll',
    'mscms.dll', 'pdh.dll', 'wbemuuid.dll', 'wsdapi.dll', 'devobj.dll', 'cryptnet.dll', 'dxgidebug.dll',
    'usp10.dll', 'winscard.dll', 'xmllite.dll',
}
# Installed with a graphics driver, not always there: the plugin that needs
# it (libplacebo's Vulkan output...) is skipped where it is missing, as in
# VideoLAN's own packages
DRIVER_DLLS = {'vulkan-1.dll', 'nvcuda.dll', 'opencl.dll'}


def is_system(name):
    n = name.lower()
    if n.startswith(('api-ms-win-', 'ext-ms-win-')):
        return True
    if n in SYSTEM_DLLS:
        return True
    return False


def imports_of(pe):
    names = []
    for attr in ('DIRECTORY_ENTRY_IMPORT', 'DIRECTORY_ENTRY_DELAY_IMPORT'):
        for entry in getattr(pe, attr, []) or []:
            dll = entry.dll.decode('ascii', 'replace') if isinstance(entry.dll, bytes) else entry.dll
            names.append((dll, attr == 'DIRECTORY_ENTRY_DELAY_IMPORT'))
    return names


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('package')
    ap.add_argument('--arch', choices=['arm64', 'x64'], default='arm64')
    ap.add_argument('--no-iptv', action='store_true', help='a package made without the Live TV add-on')
    args = ap.parse_args()
    root = os.path.abspath(args.package)
    want = 0xAA64 if args.arch == 'arm64' else 0x8664
    failures = 0

    def fail(text):
        nonlocal failures
        failures += 1
        print('FAIL: ' + text)

    binaries = []
    for d, dirs, files in os.walk(root):
        rel = os.path.relpath(d, root)
        # (a copy's own profile, the files an update keeps aside)
        if rel.split(os.sep)[0] == 'portable':
            dirs[:] = []
            continue
        for f in files:
            if f.lower().endswith(('.exe', '.dll')) and '.old-' not in f:
                binaries.append(os.path.join(d, f))
    top = {f.lower() for f in os.listdir(root) if f.lower().endswith('.dll')}

    wrong = []
    missing = {}
    drivers = {}
    system_used = set()
    for path in sorted(binaries):
        rel = os.path.relpath(path, root).replace(os.sep, '/')
        try:
            pe = pefile.PE(path, fast_load=True)
        except pefile.PEFormatError as e:
            wrong.append('%s (not a PE file: %s)' % (rel, e))
            continue
        machine = pe.FILE_HEADER.Machine
        if machine != want:
            wrong.append('%s (%s)' % (rel, MACHINES.get(machine, hex(machine))))
        pe.parse_data_directories(directories=[
            pefile.DIRECTORY_ENTRY['IMAGE_DIRECTORY_ENTRY_IMPORT'],
            pefile.DIRECTORY_ENTRY['IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT']])
        here = {f.lower() for f in os.listdir(os.path.dirname(path))}
        for dll, delayed in imports_of(pe):
            n = dll.lower()
            if n in here or n in top:
                continue
            if is_system(n):
                system_used.add(n)
                continue
            if n in DRIVER_DLLS:
                drivers.setdefault(n, []).append(rel)
                continue
            missing.setdefault(n, []).append(rel + (' (delay-loaded)' if delayed else ''))
        pe.close()

    if len(binaries) < 300:
        fail('only %d binaries in %s' % (len(binaries), root))
    if wrong:
        fail('%d of %d binaries are not %s' % (len(wrong), len(binaries), MACHINES[want]))
        for w in wrong[:30]:
            print('  ' + w)
    else:
        print('ok: the %d binaries are %s (machine 0x%04X)' % (len(binaries), MACHINES[want], want))
    if missing:
        fail('%d imported DLLs neither in the package nor in Windows' % len(missing))
        for n, users in sorted(missing.items()):
            print('  %s: %s' % (n, ', '.join(users[:5]) + (' ...' if len(users) > 5 else '')))
    else:
        print('ok: every import found (the package, or Windows: %d system DLLs)' % len(system_used))
    for n, users in sorted(drivers.items()):
        print('note: %s (from a graphics driver) for %s' % (n, ', '.join(users)))
    print('  Windows DLLs used: ' + ', '.join(sorted(n for n in system_used if not n.startswith(('api-ms-', 'ext-ms-')))))

    # 3. The parts every copy needs
    def need(rel, what):
        if not os.path.isfile(os.path.join(root, rel)):
            fail('no %s (%s)' % (what, rel))
            return False
        return True

    for rel in ('vlc.exe', 'libvlc.dll', 'libvlccore.dll', 'vlc-cache-gen.exe', 'vlc-preparser.exe',
                'vlc-redesign-update.exe', 'plugins/gui/libqt_plugin.dll',
                'plugins/video_output/libdirect3d11_plugin.dll', 'plugins/audio_output/libmmdevice_plugin.dll',
                'plugins/codec/libavcodec_plugin.dll', 'plugins/lua/liblua_plugin.dll'):
        need(rel, 'part of the package')
    qt = os.path.join(root, 'plugins/gui/libqt_plugin.dll')
    if os.path.isfile(qt):
        qt_dlls = sorted(f for f in top if f.startswith('qt'))
        if qt_dlls:
            fail('Qt DLLs in the package (Qt is built into the interface): ' + ', '.join(qt_dlls))
        data = open(qt, 'rb').read()
        # the interface's QML and Qt's own QML modules and platform, built in
        found = [t for t in ('MainInterface.qml', 'qwindows', 'QtQuick/Controls', 'VLC/MainInterface')
                 if t.encode('utf-16-le') in data or t.encode() in data]
        if len(found) >= 3:
            print('ok: Qt and the QML built into libqt_plugin.dll (%s)' % ', '.join(found))
        else:
            fail('libqt_plugin.dll: Qt or its QML not found built in (only %s)' % (found or 'none'))
        badge = 'Windows ARM64' if args.arch == 'arm64' else 'Windows x64'
        other = 'Windows x64' if args.arch == 'arm64' else 'Windows ARM64'
        if badge.encode('utf-16-le') in data and other.encode('utf-16-le') not in data:
            print('ok: About badge "%s"' % badge)
        else:
            fail('the About badge is not "%s" alone' % badge)
    iptv = os.path.join(root, 'plugins/misc/libiptv_plugin.dll')
    if args.no_iptv:
        if os.path.isfile(iptv):
            fail('--no-iptv: the Live TV add-on is in the package')
    elif need('plugins/misc/libiptv_plugin.dll', 'Live TV add-on'):
        print('ok: Live TV add-on (plugins/misc/libiptv_plugin.dll, %d KB)' % (os.path.getsize(iptv) // 1024))
    mo = [d for d in os.listdir(os.path.join(root, 'locale'))
          if os.path.isfile(os.path.join(root, 'locale', d, 'LC_MESSAGES', 'vlc.mo'))] \
        if os.path.isdir(os.path.join(root, 'locale')) else []
    if len(mo) >= 20:
        print('ok: %d translations' % len(mo))
    else:
        fail('only %d translations (locale/*/LC_MESSAGES/vlc.mo)' % len(mo))
    if not os.path.isdir(os.path.join(root, 'lua', 'intf')):
        fail('no Lua scripts (lua/intf)')
    credits = os.path.join(root, 'CREDITS.txt')
    if os.path.isfile(credits) and 'Juxtopposed' in open(credits, encoding='utf-8', errors='replace').read():
        print('ok: designer credited (CREDITS.txt)')
    else:
        fail('CREDITS.txt does not name Juxtopposed')
    for rel in ('licenses/Inter-OFL.txt', 'licenses/DMMono-OFL.txt', 'COPYING.txt'):
        need(rel, 'licence')

    print('failures: %d' % failures)
    return failures


if __name__ == '__main__':
    sys.exit(main())
