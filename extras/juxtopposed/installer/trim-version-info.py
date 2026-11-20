#!/usr/bin/env python3
# Copyright (C) 2026 VLC authors and VideoLAN
# SPDX-License-Identifier: GPL-2.0-or-later
"""Trims the spaces Inno Setup leaves after the installer's version strings.

Inno Setup writes the version strings of the installer (ProductName,
ProductVersion, CompanyName...) over fixed-size fields of its loader's
version resource and leaves the rest of each field as spaces: the installer
says "VLC Redesign" followed by 48 spaces. SignPath compares the product name
and version of every signed file with the artifact configuration, exactly,
and SignPath Foundation requires that check (docs/code-signing.md), so the
installer's values are trimmed here, in place, as Inno Setup itself wrote
them: the end of each value becomes NUL characters and its length is
corrected. The file keeps its size and every byte outside the version
resource, so the installer's data appended after the loader stays where the
loader looks for it.

Usage: trim-version-info.py SETUP.EXE [--check]
  --check   change nothing; exit 1 if a value still ends in spaces
"""

import struct
import sys

RT_VERSION = 16


def fail(msg):
    sys.exit(f'trim-version-info: {msg}')


def u16(b, o):
    return struct.unpack_from('<H', b, o)[0]


def u32(b, o):
    return struct.unpack_from('<I', b, o)[0]


def align4(o):
    return (o + 3) & ~3


def version_resource(b):
    """(file offset, size) of the first RT_VERSION resource."""
    if b[:2] != b'MZ':
        fail('not an .exe')
    pe = u32(b, 0x3C)
    if b[pe:pe + 4] != b'PE\0\0':
        fail('no PE header')
    coff = pe + 4
    nsections = u16(b, coff + 2)
    opt = coff + 20
    opt_size = u16(b, coff + 16)
    magic = u16(b, opt)
    if magic == 0x10B:
        dirs = opt + 96
    elif magic == 0x20B:
        dirs = opt + 112
    else:
        fail(f'unknown optional header {magic:#x}')
    rsrc_rva = u32(b, dirs + 2 * 8)
    if not rsrc_rva:
        fail('no resources')
    sections = []
    for i in range(nsections):
        s = opt + opt_size + 40 * i
        vsize, va, rawsize, rawptr = struct.unpack_from('<IIII', b, s + 8)
        sections.append((va, max(vsize, rawsize), rawptr))

    def off(rva):
        for va, size, rawptr in sections:
            if va <= rva < va + size:
                return rawptr + rva - va
        fail(f'RVA {rva:#x} in no section')

    base = off(rsrc_rva)

    def entries(d):
        named, ids = u16(b, d + 12), u16(b, d + 14)
        for i in range(named + ids):
            e = d + 16 + 8 * i
            yield u32(b, e), u32(b, e + 4)

    for name, target in entries(base):
        if name == RT_VERSION and target & 0x80000000:
            d = base + (target & 0x7FFFFFFF)
            while True:  # first name, first language
                name2, target2 = next(entries(d))
                if not target2 & 0x80000000:
                    data = base + target2
                    return off(u32(b, data)), u32(b, data + 4)
                d = base + (target2 & 0x7FFFFFFF)
    fail('no version resource')


def nodes(b, start, end):
    """The child nodes (offset, wLength) between start and end."""
    o = align4(start)
    while o + 6 <= end:
        length = u16(b, o)
        if length == 0:
            break
        yield o, length
        o = align4(o + length)


def key_end(b, o):
    """Offset after the node's NUL-terminated UTF-16 key."""
    k = o + 6
    while u16(b, k):
        k += 2
    return k + 2


def key(b, o):
    return bytes(b[o + 6:key_end(b, o) - 2]).decode('utf-16-le')


def strings(b, res, size):
    """(node offset, value offset, value chars) of every String node."""
    root_end = res + u16(b, res)
    if key(b, res) != 'VS_VERSION_INFO' or root_end > res + size:
        fail('unexpected version resource')
    fixed = align4(key_end(b, res))
    for child, length in nodes(b, fixed + u16(b, res + 2), root_end):
        if key(b, child) != 'StringFileInfo':
            continue
        for table, tlen in nodes(b, key_end(b, child), child + length):
            for s, slen in nodes(b, key_end(b, table), table + tlen):
                value = align4(key_end(b, s))
                chars = (s + slen - value) // 2
                yield s, value, chars


def main():
    args = [a for a in sys.argv[1:] if a != '--check']
    check = '--check' in sys.argv[1:]
    if len(args) != 1:
        fail('usage: trim-version-info.py SETUP.EXE [--check]')
    path = args[0]
    with open(path, 'rb') as f:
        b = bytearray(f.read())
    size_before = len(b)
    res, size = version_resource(b)
    padded = []
    for s, value, chars in strings(b, res, size):
        text = bytes(b[value:value + 2 * chars]).decode('utf-16-le').split('\0', 1)[0]
        name = key(b, s)
        if text != text.rstrip(' '):
            padded.append(name)
            if check:
                continue
            trimmed = text.rstrip(' ')
            new = (trimmed + '\0').encode('utf-16-le')
            b[value:value + 2 * chars] = new + bytes(2 * chars - len(new))
            struct.pack_into('<H', b, s + 2, len(trimmed) + 1)
            print(f'{name}: "{trimmed}"')
    if check:
        if padded:
            fail(f'{path}: padded with spaces: {", ".join(padded)}')
        print(f'{path}: no padded value')
        return
    if len(b) != size_before:
        fail('the size changed')
    with open(path, 'r+b') as f:
        f.write(b)
    print(f'{path}: {len(padded)} value(s) trimmed')


if __name__ == '__main__':
    main()
