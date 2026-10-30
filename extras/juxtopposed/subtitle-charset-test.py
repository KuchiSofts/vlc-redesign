#!/usr/bin/env python3
# Copyright (C) 2026 VLC authors and VideoLAN. GPL version 2 or later.
#
# Checks the encoding detection of text subtitles over the corpus:
# regenerates it, builds extras/juxtopposed/subtitle-charset.c with the
# detector, runs it and compares every decoded file with its .expected text
# (Unicode NFC). Every character must be right: no '?', no U+FFFD.
#   python3 extras/juxtopposed/subtitle-charset-test.py [--cc "gcc"] [--exe path]
# --exe runs an already built tool (the Windows one, built with MinGW and
# GNU libiconv as VLC is: extras/juxtopposed/subtitle-charset-win64.sh).
import argparse
import os
import shlex
import subprocess
import sys
import tempfile
import unicodedata

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, '..', '..'))
FIXTURES = os.path.join(HERE, 'fixtures', 'subtitles')


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--cc', default='cc')
    ap.add_argument('--exe')
    ap.add_argument('--corpus')
    args = ap.parse_args()

    corpus = args.corpus or os.path.join(tempfile.mkdtemp(prefix='subcs-'), 'corpus')
    subprocess.check_call([sys.executable, os.path.join(FIXTURES, 'make-corpus.py'), corpus])
    out = tempfile.mkdtemp(prefix='subcs-out-')
    exe = args.exe
    if not exe:
        exe = os.path.join(out, 'subtitle-charset')
        cmd = shlex.split(args.cc) + [
            '-std=gnu11', '-O1', '-Wall', '-Wextra', '-DSUBS_CHARSET_STANDALONE',
            '-I' + os.path.join(ROOT, 'modules', 'codec'),
            os.path.join(HERE, 'subtitle-charset.c'),
            os.path.join(ROOT, 'modules', 'codec', 'subs_charset.c'), '-o', exe]
        if sys.platform == 'darwin':
            cmd.append('-liconv')
        subprocess.check_call(cmd)

    files = sorted(f for f in os.listdir(corpus) if not f.endswith('.expected'))
    res = subprocess.run([exe, out] + [os.path.join(corpus, f) for f in files],
                         capture_output=True, text=True, encoding='utf-8')
    charsets = dict(line.rsplit('\t', 1) for line in res.stdout.splitlines() if '\t' in line)
    failures = 0
    for f in files:
        expected = open(os.path.join(corpus, f + '.expected'), 'rb').read().decode('utf-8')
        try:
            got = open(os.path.join(out, f + '.out'), 'rb').read().decode('utf-8')
        except (OSError, UnicodeDecodeError) as e:
            print('FAIL %-34s %s' % (f, e))
            failures += 1
            continue
        cs = charsets.get(os.path.join(corpus, f), '?')
        if unicodedata.normalize('NFC', got) != unicodedata.normalize('NFC', expected):
            failures += 1
            bad = next((i for i, (a, b) in enumerate(zip(unicodedata.normalize('NFC', got),
                                                          unicodedata.normalize('NFC', expected)))
                        if a != b), None)
            sample = unicodedata.normalize('NFC', got)
            print('FAIL %-34s read as %-10s %r' % (f, cs, sample[max(0, (bad or 0) - 10):(bad or 0) + 30]))
        elif '�' in got:
            failures += 1
            print('FAIL %-34s U+FFFD' % f)
    print('%d/%d files decoded right' % (len(files) - failures, len(files)))
    if res.stderr.strip():
        print(res.stderr.strip())
    return 1 if failures else 0


if __name__ == '__main__':
    sys.exit(main())
