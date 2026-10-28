#!/usr/bin/env python3
# Copyright (C) 2026 VLC authors and VideoLAN
# SPDX-License-Identifier: GPL-2.0-or-later
# The catalog of the build (catalog/catalog.json) as a string of the plugin
# (meson; Makefile.am does the same with a shell rule)
import sys

with open(sys.argv[1], encoding='utf-8') as source:
    text = source.read()
with open(sys.argv[2], 'w', encoding='utf-8') as header:
    header.write('static const char iptv_catalog[] = R"vlcjson(\n' + text + '\n)vlcjson";\n')
