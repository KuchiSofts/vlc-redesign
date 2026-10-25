# Copyright (C) 2026 VLC authors and VideoLAN
# SPDX-License-Identifier: GPL-2.0-or-later
"""Applies a .vrdelta to a copy of VLC Redesign (see make-delta.py), as the
app does, to check a release's deltas before publishing them.

  python apply-delta.py <delta> <folder of the old version> [--check <folder of the new version>]

Every new file is checked against its SHA-256 before the folder is
changed; --check also compares each result with the new release's file.
"""
import hashlib, json, os, struct, sys, zlib


def quncompress(block):
    size = struct.unpack('>I', block[:4])[0]
    data = zlib.decompress(block[4:])
    assert len(data) == size, 'bad block size'
    return data


def bspatch(old, control, diff, extra, size):
    new = bytearray(size)
    newpos = oldpos = di = ei = 0
    for i in range(0, len(control), 24):
        x, y, z = struct.unpack_from('<qqq', control, i)
        if x < 0 or y < 0 or newpos + x + y > size:
            raise ValueError('corrupt patch')
        # x bytes: the old ones plus the difference
        for k in range(x):
            o = old[oldpos + k] if 0 <= oldpos + k < len(old) else 0
            new[newpos + k] = (diff[di + k] + o) & 0xFF
        newpos += x
        oldpos += x
        di += x
        new[newpos:newpos + y] = extra[ei:ei + y]
        newpos += y
        ei += y
        oldpos += z
    if newpos != size:
        raise ValueError('patch too short')
    return bytes(new)


def read(path):
    data = open(path, 'rb').read()
    if data[:8] != b'VRDELTA1':
        raise ValueError('not a VLC Redesign delta')
    n = struct.unpack_from('<I', data, 8)[0]
    header = json.loads(data[12:12 + n])
    return header, memoryview(data)[12 + n:]


def main():
    delta, folder = sys.argv[1], sys.argv[2]
    check = sys.argv[sys.argv.index('--check') + 1] if '--check' in sys.argv else None
    header, blobs = read(delta)
    results = {}
    for e in header['files']:
        o = e['offset']
        if e['kind'] == 'full':
            data = quncompress(bytes(blobs[o:o + e['length']]))
        else:
            old = open(os.path.join(folder, e['path']), 'rb').read()
            if hashlib.sha256(old).hexdigest() != e['old_sha256']:
                raise SystemExit(f'{e["path"]}: not the file this patch applies to')
            c, d, x = e['parts']
            data = bspatch(old, quncompress(bytes(blobs[o:o + c])), quncompress(bytes(blobs[o + c:o + c + d])),
                           quncompress(bytes(blobs[o + c + d:o + c + d + x])), e['size'])
        if hashlib.sha256(data).hexdigest() != e['sha256']:
            raise SystemExit(f'{e["path"]}: wrong SHA-256 after applying')
        if check and open(os.path.join(check, e['path']), 'rb').read() != data:
            raise SystemExit(f'{e["path"]}: differs from the new release')
        results[e['path']] = data
    for rel, data in results.items():
        # a file in a folder the old release lacks (0.3.0's locale\ for 0.2.x),
        # as the app's updater makes its folders (make_parents)
        os.makedirs(os.path.dirname(os.path.join(folder, rel)), exist_ok=True)
        open(os.path.join(folder, rel), 'wb').write(data)
    for rel in header['removed']:
        p = os.path.join(folder, rel)
        if os.path.exists(p):
            os.remove(p)
    print(f'{header["from"]} -> {header["to"]}: {len(results)} files written, {len(header["removed"])} removed, all checked')


if __name__ == '__main__':
    main()
