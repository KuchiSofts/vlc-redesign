#!/usr/bin/env python3
"""The Weblate catalogs of VLC Redesign.

VLC's own texts are translated by VideoLAN and arrive with each upstream sync;
only the texts VLC Redesign adds are offered on Weblate. They live in
po/redesign: redesign.pot and one <lang>.po per language, cut out of the full
catalogs po/<lang>.po.

  weblate-sync.py export [--upstream REF] [--min PERCENT] [LANG ...]
      po/<lang>.po -> po/redesign/<lang>.po. The texts of VLC Redesign are
      those of po/vlc.pot that the vlc.pot of REF (default: master, upstream
      VLC) does not have and that a file changed since REF uses. Without LANG, the languages already in po/redesign,
      plus every language with at least PERCENT (default 90) of them
      translated.
  weblate-sync.py import
      po/redesign/<lang>.po -> po/<lang>.po: the translations made on Weblate
      replace those of the full catalogs (the source references stay).
  weblate-sync.py stats [--upstream REF]
      How much of the texts of VLC Redesign each language has, and how many
      Weblate strings each would cost.

Run export after po/vlc.pot and the catalogs were updated (potgen, msgmerge),
and import before a build when Weblate pushed new translations. Python only:
no gettext tools needed.
"""
import argparse
import os
import re
import subprocess
import sys

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..', '..'))
PO = os.path.join(ROOT, 'po')
OUT = os.path.join(PO, 'redesign')


class Entry:
    """One entry of a PO file, kept as its lines."""

    def __init__(self, lines):
        self.lines = lines
        self.comments = [l for l in lines if l.startswith('#') and not l.startswith('#,')]
        self.flags = [l for l in lines if l.startswith('#,')]
        self.body = [l for l in lines if not l.startswith('#')]
        self.key = (self._field('msgctxt'), self._field('msgid'))

    def _field(self, name):
        text, inside = None, False
        for l in self.body:
            m = re.match(r'(msgctxt|msgid|msgid_plural|msgstr(?:\[\d+\])?) (".*")$', l)
            if m:
                inside = m.group(1) == name
                if inside:
                    text = m.group(2)[1:-1]
            elif inside and l.startswith('"'):
                text += l[1:-1]
        return text

    def files(self):
        return [ref.rsplit(':', 1)[0] for l in self.comments if l.startswith('#:') for ref in l[2:].split()]

    def translated(self):
        strs = []
        inside = False
        for l in self.body:
            m = re.match(r'(msgstr(?:\[\d+\])?) "(.*)"$', l)
            if m:
                inside = True
                strs.append(m.group(2))
            elif re.match(r'msg', l):
                inside = False
            elif inside and l.startswith('"'):
                strs[-1] += l[1:-1]
        fuzzy = any('fuzzy' in f for f in self.flags)
        return bool(strs) and all(strs) and not fuzzy

    def text(self):
        return '\n'.join(self.lines)


def read(path):
    """The header and the entries of a PO file (obsolete entries left out)."""
    with open(path, encoding='utf-8') as f:
        blocks = re.split(r'\n\s*\n', f.read().strip('\n'))
    entries = []
    header = None
    for b in blocks:
        lines = b.split('\n')
        if all(l.startswith('#~') or l.startswith('#') for l in lines) and any(l.startswith('#~') for l in lines):
            continue
        e = Entry(lines)
        if e.key == (None, '') and header is None:
            header = e
        elif e.key[1] is not None:
            entries.append(e)
    return header, entries


def write(path, header, entries):
    with open(path, 'w', encoding='utf-8', newline='\n') as f:
        f.write('\n\n'.join([header.text()] + [e.text() for e in entries]) + '\n')


def redesign_keys(upstream):
    """The (msgctxt, msgid) of the texts VLC Redesign adds, in vlc.pot order."""
    old = subprocess.run(['git', '-C', ROOT, 'show', '%s:po/vlc.pot' % upstream],
                         capture_output=True, check=True).stdout.decode('utf-8', 'replace')
    tmp = os.path.join(OUT, '.upstream.pot')
    os.makedirs(OUT, exist_ok=True)
    with open(tmp, 'w', encoding='utf-8') as f:
        f.write(old)
    try:
        known = {e.key for e in read(tmp)[1]}
    finally:
        os.remove(tmp)
    # The vlc.pot of upstream lags behind its code: a text it lacks may still
    # be VLC's. A text of VLC Redesign is also used by a file it changed.
    changed = set(subprocess.run(['git', '-C', ROOT, 'diff', '--name-only', '%s...HEAD' % upstream],
                                 capture_output=True, check=True, text=True).stdout.split())
    header, pot = read(os.path.join(PO, 'vlc.pot'))
    return header, [e for e in pot if e.key not in known and changed.intersection(e.files())]


def languages():
    with open(os.path.join(PO, 'LINGUAS'), encoding='utf-8') as f:
        return [w for l in f if not l.startswith('#') for w in l.split()]


def coverage(lang, keys):
    _, entries = read(os.path.join(PO, lang + '.po'))
    have = {e.key: e for e in entries}
    done = sum(1 for k in keys if k in have and have[k].translated())
    return have, done


def export(args):
    header, pot = redesign_keys(args.upstream)
    keys = [e.key for e in pot]
    write(os.path.join(OUT, 'redesign.pot'), header, pot)
    langs = args.langs
    if not langs:
        present = {f[:-3] for f in os.listdir(OUT) if f.endswith('.po')}
        langs = [l for l in languages()
                 if l in present or coverage(l, keys)[1] * 100 >= args.min * len(keys)]
    for lang in langs:
        lheader, lentries = read(os.path.join(PO, lang + '.po'))
        have = {e.key: e for e in lentries}
        out = [have.get(e.key, e) for e in pot]
        write(os.path.join(OUT, lang + '.po'), lheader, out)
    print('%d texts, %d languages: %s' % (len(pot), len(langs), ' '.join(langs)))
    print('Weblate strings: %d' % (len(pot) * (len(langs) + 1)))


def import_(args):
    changed = 0
    for name in sorted(os.listdir(OUT)):
        if not name.endswith('.po'):
            continue
        lang = name[:-3]
        full = os.path.join(PO, name)
        if not os.path.exists(full):
            print('%s: new on Weblate, no po/%s yet: add it to po/LINGUAS and msgmerge it first' % (lang, name))
            continue
        _, done = read(os.path.join(OUT, name))
        theirs = {e.key: e for e in done}
        header, entries = read(full)
        n = 0
        for i, e in enumerate(entries):
            w = theirs.get(e.key)
            # (an empty or fuzzy one on Weblate never replaces a translation)
            if w is None or not w.translated() or (w.body == e.body and w.flags == e.flags):
                continue
            entries[i] = Entry(e.comments + w.flags + w.body)
            n += 1
        if n:
            write(full, header, entries)
            changed += 1
        print('%s: %d translations taken' % (lang, n))
    print('%d catalogs changed' % changed)


def stats(args):
    _, pot = redesign_keys(args.upstream)
    keys = [e.key for e in pot]
    for lang in languages():
        done = coverage(lang, keys)[1]
        print('%-8s %5.1f%%  %d/%d' % (lang, done * 100.0 / len(keys), done, len(keys)))
    print('%d texts; each language costs %d Weblate strings' % (len(keys), len(keys)))


def main():
    p = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = p.add_subparsers(dest='cmd', required=True)
    e = sub.add_parser('export')
    e.add_argument('--upstream', default='master')
    e.add_argument('--min', type=float, default=90)
    e.add_argument('langs', nargs='*')
    sub.add_parser('import')
    s = sub.add_parser('stats')
    s.add_argument('--upstream', default='master')
    a = p.parse_args()
    {'export': export, 'import': import_, 'stats': stats}[a.cmd](a)


if __name__ == '__main__':
    sys.exit(main())
