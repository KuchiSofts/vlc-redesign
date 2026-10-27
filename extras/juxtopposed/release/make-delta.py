# Copyright (C) 2026 VLC authors and VideoLAN
# SPDX-License-Identifier: GPL-2.0-or-later
"""Small updates of VLC Redesign: only what changed since an earlier release.

  python make-delta.py normalize <package>
      Sets the PE time stamp of every .dll and .exe to 0 (and computes the
      checksum again): a binary whose code didn't change then stays the same,
      byte for byte, from one release to the next.

  python make-delta.py deltas <package> <version> <dist> [--keep N]
      For each of the N latest earlier releases whose zip is in <dist>
      (default 3, from 0.2.4 on: the first that applies them), writes <dist>/vlc-redesign-<version>-delta-from-<old>.vrdelta:
      the files that changed (a binary patch when it's much smaller), the
      files that went away, and the SHA-256 of every new file. The app checks
      and applies it (modules/gui/qt/util/redesign_updater.cpp).

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
PATCH_MIN = 64 * 1024        # smaller files go whole
PATCH_GAIN = 0.6             # a patch only if it's under 60 % of the whole file
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


def make_delta(package, new_files, old_version, old_files, version, out_path):
    import bsdiff4.core as bsdiff
    header = {'format': 1, 'from': old_version, 'to': version,
              'removed': sorted(p for p in old_files if p not in new_files), 'files': []}
    blobs = io.BytesIO()
    patched = whole = 0
    for rel in sorted(new_files):
        data = open(new_files[rel], 'rb').read()
        old = old_files.get(rel)
        if old is not None and comparable(old) == comparable(data):
            continue                       # same code: the file in place stays
        entry = {'path': rel, 'size': len(data), 'sha256': sha256(data), 'offset': blobs.tell()}
        full = qcompress(data)
        if old is not None and len(data) >= PATCH_MIN:
            control, diff, extra = bsdiff.diff(old, data)
            parts = [qcompress(b''.join(struct.pack('<qqq', *t) for t in control)), qcompress(diff), qcompress(extra)]
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


def deltas(package, version, dist, keep):
    new_files = package_files(package)
    olds = []
    for n in os.listdir(dist):
        m = re.match(r'vlc-redesign-(.+)-win64\.zip$', n)
        if (m and m.group(1) != version and version_key(m.group(1)) < version_key(version)
                and version_key(m.group(1)) >= version_key(FIRST_APPLIER)):
            olds.append(m.group(1))
    olds.sort(key=version_key)
    for old in olds[-keep:]:
        make_delta(package, new_files, old, zip_files(os.path.join(dist, f'vlc-redesign-{old}-win64.zip')),
                   version, os.path.join(dist, f'vlc-redesign-{version}-delta-from-{old}.vrdelta'))


if __name__ == '__main__':
    if len(sys.argv) >= 3 and sys.argv[1] == 'normalize':
        normalize(sys.argv[2])
    elif len(sys.argv) >= 5 and sys.argv[1] == 'deltas':
        keep = int(sys.argv[sys.argv.index('--keep') + 1]) if '--keep' in sys.argv else 3
        deltas(sys.argv[2], sys.argv[3], sys.argv[4], keep)
    else:
        sys.exit(__doc__)
