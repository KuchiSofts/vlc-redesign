# Copyright (C) 2026 VLC authors and VideoLAN
# SPDX-License-Identifier: GPL-2.0-or-later
"""Small updates of VLC Redesign: only what changed since an earlier release.

  python make-delta.py normalize <package>
      Sets the PE time stamp of every .dll and .exe to 0 (and computes the
      checksum again): a binary whose code didn't change then stays the same,
      byte for byte, from one release to the next.

  python make-delta.py deltas <package> <version> <dist> [--keep N] [--since V] [--platform win64|winarm64]
      Writes <dist>/vlc-redesign-<version>-delta-from-<old>.vrdelta from the
      release before this one (the newest older zip in <dist>): the files
      that changed, as binary patches, the files that went away, and the
      SHA-256 of every new file. The app checks and applies it
      (modules/gui/qt/util/redesign_updater.cpp), and from 0.7.0 on finds its
      way to the newest release through the ones between them (0.7.0 ->
      0.7.1 -> 0.7.2): ONE small update a release, whatever version a copy
      has, not one for every older version (0.6.9 carried nine of them for
      each architecture, half an hour to make).
      --platform winarm64: from the vlc-redesign-<old>-winarm64.zip files, to
      vlc-redesign-<version>-winarm64-delta-from-<old>.vrdelta (win64, the
      default, keeps the name without the platform).
      --keep N: from the N latest earlier releases instead (default 1);
      --since V: from every earlier release from V on. Only for a release
      that must reach copies whose updater can't follow the way (before
      0.7.0): each one more is a file on the release page.

The .vrdelta format (all integers little-endian):
  "VRDELTA1", u32 header length, the header (UTF-8 JSON), then the data.
  Header: {"format": 1, "from", "to", "removed": [path...],
           "files": [{"path", "size", "sha256", "kind": "full", "offset", "length"}
                     or {..., "kind": "patch", "old_sha256", "offset", "parts": [c, d, e]}]}
  Every block of data is in the qCompress format (u32 big-endian size, then
  zlib). A patch is bsdiff's: its control triples (three i64 each), its
  difference bytes and its extra bytes, one block each. Paths use "/" and
  are relative to the folder of vlc.exe.

Needs bsdiff4 (pip install bsdiff4).
"""
import hashlib, io, json, os, re, struct, sys, zipfile, zlib

# (.update/: the working folder of the app's own updater, if a package
# folder was run before it was packed; the app refuses a delta that names it)
SKIP = re.compile(r'^(portable/|\.update/|plugins/plugins\.dat$)|\.old-', re.I)
PATCH_GAIN = 0.6             # a patch only if it's under 60 % of the whole file
# A binary keeps its size and changes in a few places at every release (the
# version in its resources, their time, its checksum: 9 or 10 bytes in each
# of 413 files from 0.6.8 to 0.6.9). Such a file gets its patch without
# bsdiff's suffix sort: the difference to add to the whole old file.
STAMP_BLOCK = 4096
STAMP_BLOCKS = 64            # at most so many blocks that differ; more: bsdiff
FIRST_APPLIER = '0.2.4'      # older releases can't apply a delta: they get the installer


def pe_offsets(data):
    """Offsets of the time stamp and the checksum of a PE image, or None."""
    if len(data) < 0x100 or data[:2] != b'MZ':
        return None
    pe = struct.unpack_from('<I', data, 0x3C)[0]
    if pe + 0x60 > len(data) or data[pe:pe + 4] != b'PE\0\0':
        return None
    return pe + 8, pe + 24 + 64


def pe_checksum(data, checksum_at):
    total = 0
    for i in range(0, len(data) - 1, 2):
        if i in (checksum_at, checksum_at + 2):
            continue
        total += data[i] | (data[i + 1] << 8)
        total = (total & 0xFFFF) + (total >> 16)
    if len(data) % 2:
        total += data[-1]
        total = (total & 0xFFFF) + (total >> 16)
    total = (total & 0xFFFF) + (total >> 16)
    return (total + len(data)) & 0xFFFFFFFF


def normalized(data):
    """The image with its time stamp at 0 and its checksum computed again."""
    offsets = pe_offsets(data)
    if not offsets:
        return data
    stamp, checksum = offsets
    b = bytearray(data)
    struct.pack_into('<I', b, stamp, 0)
    struct.pack_into('<I', b, checksum, 0)
    struct.pack_into('<I', b, checksum, pe_checksum(b, checksum))
    return bytes(b)


def comparable(data):
    """What two builds of the same code share: the image without its time stamp and checksum."""
    offsets = pe_offsets(data)
    if not offsets:
        return data
    b = bytearray(data)
    for o in offsets:
        struct.pack_into('<I', b, o, 0)
    return bytes(b)


def sha256(data):
    return hashlib.sha256(data).hexdigest()


def qcompress(data):
    return struct.pack('>I', len(data)) + zlib.compress(data, 9)


def package_files(root):
    files = {}
    for dirpath, _, names in os.walk(root):
        for n in names:
            path = os.path.join(dirpath, n)
            rel = os.path.relpath(path, root).replace(os.sep, '/')
            if not SKIP.search(rel):
                files[rel] = path
    return files


def zip_files(path):
    with zipfile.ZipFile(path) as z:
        names = [i.filename for i in z.infolist() if not i.is_dir()]
        prefix = os.path.commonprefix(names)
        prefix = prefix[:prefix.rfind('/') + 1] if '/' in prefix else ''
        out = {}
        for i in z.infolist():
            if i.is_dir():
                continue
            rel = i.filename[len(prefix):]
            if rel and not SKIP.search(rel):
                out[rel] = z.read(i)
        return out


def version_key(v):
    """The order of the app (RedesignUpdater::versionLess): the numbers, then
    a pre-release before its release, its parts as SemVer compares them
    (numbers as numbers, before words; fewer parts first)."""
    v = v.strip().lstrip('vV')
    core, _, pre = v.partition('-')
    numbers = [int(x) if x.isdigit() else 0 for x in core.split('.')]
    numbers += [0] * (3 - len(numbers))
    while len(numbers) > 3 and numbers[-1] == 0:
        numbers.pop()
    parts = tuple((0, int(x), '') if x.isdigit() else (1, 0, x.lower()) for x in pre.split('.')) if pre else ()
    return (tuple(numbers), not pre, parts)


def normalize(package):
    count = 0
    for rel, path in package_files(package).items():
        if not rel.lower().endswith(('.dll', '.exe')):
            continue
        data = open(path, 'rb').read()
        new = normalized(data)
        if new != data:
            open(path, 'wb').write(new)
            count += 1
    print(f'time stamps set to 0 in {count} binaries')


def control_block(control):
    """bsdiff4's control triples as the app reads them: three little-endian
    i64 each. Up to 1.2 it gave a list of tuples; since 1.4 it gives the bytes
    of a BSDIFF40 file, where a negative number has its top bit set over its
    size (sign and magnitude) instead of being a two's complement."""
    if isinstance(control, (bytes, bytearray, memoryview)):
        control = bytes(control)
        if len(control) % 24:
            raise ValueError('bsdiff4: control block is not whole triples')
        values = []
        for i in range(0, len(control), 8):
            v = int.from_bytes(control[i:i + 8], 'little')
            values.append(-(v & (2**63 - 1)) if v >> 63 else v)
        return struct.pack(f'<{len(values)}q', *values)
    return b''.join(struct.pack('<qqq', *t) for t in control)


def on_big_stack(work, *args):
    """work(*args) on a thread with a large stack (255 MB: Python on Windows
    takes less than 256): bsdiff4's suffix sort recurses, and on a big file
    (libavcodec's 31 MB, from 0.5.8 to 0.6.0) it overflowed the 1 MB stack of
    Python's main thread on Windows (the process ended with 0xC00000FD and no
    message; 64 MB was enough for it)."""
    import threading
    result = {}

    def run():
        try:
            result['value'] = work(*args)
        except BaseException as e:      # (raised again by the caller)
            result['error'] = e

    before = threading.stack_size()
    for megabytes in (255, 128, 64):
        try:
            threading.stack_size(megabytes << 20)
            break
        except ValueError:              # (more than this system gives a thread)
            continue
    try:
        thread = threading.Thread(target=run)
        thread.start()
    finally:
        threading.stack_size(before)
    thread.join()
    if 'error' in result:
        raise result['error']
    return result['value']


def stamp_patch(old, data):
    """bsdiff's three parts (one control triple: the whole file, nothing
    extra, no seek) for a file that kept its size and changed in a few blocks
    only; None when it changed more."""
    if len(old) != len(data):
        return None
    diff = bytearray(len(data))
    blocks = 0
    for i in range(0, len(data), STAMP_BLOCK):
        a, b = old[i:i + STAMP_BLOCK], data[i:i + STAMP_BLOCK]
        if a == b:
            continue
        blocks += 1
        if blocks > STAMP_BLOCKS:
            return None
        diff[i:i + len(b)] = bytes((y - x) & 0xFF for x, y in zip(a, b))
    return [(len(data), 0, 0)], bytes(diff), b''


def make_delta(package, new_files, old_version, old_files, version, out_path):
    import bsdiff4.core as bsdiff
    # (whatever its case: Windows has one file for Foo.dll and foo.dll, and a
    # file renamed in its case only must not be removed after it is written)
    kept = {p.lower() for p in new_files}
    header = {'format': 1, 'from': old_version, 'to': version,
              'removed': sorted(p for p in old_files if p.lower() not in kept), 'files': []}
    blobs = io.BytesIO()
    patched = whole = 0
    for rel in sorted(new_files):
        data = open(new_files[rel], 'rb').read()
        old = old_files.get(rel)
        if old is not None and comparable(old) == comparable(data):
            continue                       # same code: the file in place stays
        entry = {'path': rel, 'size': len(data), 'sha256': sha256(data), 'offset': blobs.tell()}
        full = qcompress(data)
        if old is not None:
            # (a small file too: the 300 plugins under 64 KB went whole for
            # their 9 bytes of version, 3 of the 4 MB of 0.6.9's update)
            control, diff, extra = stamp_patch(old, data) or on_big_stack(bsdiff.diff, old, data)
            parts = [qcompress(control_block(control)), qcompress(diff), qcompress(extra)]
            if sum(map(len, parts)) < PATCH_GAIN * len(full):
                entry.update(kind='patch', old_sha256=sha256(old), parts=[len(p) for p in parts])
                for p in parts:
                    blobs.write(p)
                header['files'].append(entry)
                patched += 1
                continue
        entry.update(kind='full', length=len(full))
        blobs.write(full)
        header['files'].append(entry)
        whole += 1
    head = json.dumps(header, separators=(',', ':')).encode()
    with open(out_path, 'wb') as f:
        f.write(b'VRDELTA1' + struct.pack('<I', len(head)) + head + blobs.getvalue())
    print(f'{os.path.basename(out_path)}: {patched} patched, {whole} whole, {len(header["removed"])} removed, '
          f'{os.path.getsize(out_path) / 2**20:.2f} MB')


def deltas(package, version, dist, keep, platform="win64", since=None):
    new_files = package_files(package)
    olds = []
    for n in os.listdir(dist):
        m = re.match(r'vlc-redesign-(.+)-' + re.escape(platform) + r'\.zip$', n)
        if (m and m.group(1) != version and version_key(m.group(1)) < version_key(version)
                and version_key(m.group(1)) >= version_key(FIRST_APPLIER)):
            olds.append(m.group(1))
    olds.sort(key=version_key)
    # the x64 deltas keep the name they had before ARM64: the copies already
    # installed look for it (redesign_updater.cpp)
    infix = '' if platform == 'win64' else '-' + platform
    chosen = olds[-keep:]
    if since:
        chosen = sorted(set(chosen) | {o for o in olds if version_key(o) >= version_key(since)}, key=version_key)
    for old in chosen:
        make_delta(package, new_files, old, zip_files(os.path.join(dist, f'vlc-redesign-{old}-{platform}.zip')),
                   version, os.path.join(dist, f'vlc-redesign-{version}{infix}-delta-from-{old}.vrdelta'))


if __name__ == '__main__':
    if len(sys.argv) >= 3 and sys.argv[1] == 'normalize':
        normalize(sys.argv[2])
    elif len(sys.argv) >= 5 and sys.argv[1] == 'deltas':
        keep = int(sys.argv[sys.argv.index('--keep') + 1]) if '--keep' in sys.argv else 1
        platform = sys.argv[sys.argv.index('--platform') + 1] if '--platform' in sys.argv else 'win64'
        if platform not in ('win64', 'winarm64'):
            sys.exit(f'Unknown platform {platform} (win64 or winarm64)')
        since = sys.argv[sys.argv.index('--since') + 1] if '--since' in sys.argv else None
        deltas(sys.argv[2], sys.argv[3], sys.argv[4], keep, platform, since)
    else:
        sys.exit(__doc__)
