#!/usr/bin/env python3
"""Renders platforms.yml (the single source of every platform of VLC Redesign,
MASTER-PLAN.md 13.3) into what shows it:

- README.md: the platform bar and the table, between <!-- platforms:start -->
  and <!-- platforms:end --> at the top;
- site/platforms.json: the data of the website, also built into the app
  (modules/gui/qt/assets.qrc, :/redesign/platforms.json) for the "Also on"
  line of About: nothing is fetched for it;
- site/index.html: the list of platforms and the download box, between their
  markers, so the page works without JavaScript too.

  python render.py              writes them (only the files that change)
  python render.py --check      fails if one of them is out of date
  python render.py --also-on windows
                                prints the "Also on" line of the release notes
  python render.py --build DIR  the website ready to publish in DIR: site/
                                and the screenshots of docs/media

Python 3.8 or later, standard library only. platforms.yml is read by a small
reader of its own, for the shape that file keeps (mappings, lists of mappings
with "- ", one "key: value" per line, full-line comments)."""
import datetime
import html
import json
import os
import shutil
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))
SOURCE = os.path.join(ROOT, 'platforms.yml')
README = os.path.join(ROOT, 'README.md')
SITE = os.path.join(ROOT, 'site')
SITE_JSON = os.path.join(SITE, 'platforms.json')
SITE_INDEX = os.path.join(SITE, 'index.html')

# status: (the table's label, its mark, the bar's word when there is no version)
STATUS = {
    'released':    ('released',    '🟢', None),
    'beta':        ('beta',        '🟡', None),
    'in-progress': ('in progress', '🟡', 'soon'),
    'planned':     ('planned',     '🔴', 'soon'),
    'research':    ('research',    '⚪', 'soon'),
    'later':       ('later',       '⚪', 'later'),
}
AVAILABLE = ('released', 'beta')

# The screenshots of the website (docs/media), copied by --build
SCREENSHOTS = [
    'docs/media/tour/home.jpg',
    'docs/media/tour/home-light.jpg',
    'docs/media/tour/seek-preview.jpg',
    'docs/media/tour/music.jpg',
    'docs/media/tour/subtitles-learn.jpg',
    'docs/media/tour/player-light.jpg',
    'docs/media/tour/customize.jpg',
    'docs/media/tour/settings.jpg',
]


# --- platforms.yml ---------------------------------------------------------

def _scalar(text):
    text = text.strip()
    if len(text) >= 2 and text[0] == text[-1] and text[0] in '"\'':
        return text[1:-1]
    return text


def parse_yaml(text):
    """The subset of YAML platforms.yml is written in"""
    lines = []
    for number, raw in enumerate(text.splitlines(), 1):
        if not raw.strip() or raw.lstrip().startswith('#'):
            continue
        if '\t' in raw[:len(raw) - len(raw.lstrip())]:
            raise ValueError('platforms.yml:%d: indent with spaces, not tabs' % number)
        lines.append([len(raw) - len(raw.lstrip(' ')), raw.strip(), number])

    def block(i, indent):
        if lines[i][1].startswith('- '):
            return sequence(i, indent)
        return mapping(i, indent)

    def mapping(i, indent):
        result = {}
        while i < len(lines) and lines[i][0] == indent and not lines[i][1].startswith('- '):
            _, content, number = lines[i]
            key, colon, value = content.partition(':')
            if not colon:
                raise ValueError('platforms.yml:%d: "key: value" expected' % number)
            key = key.strip()
            if value.strip():
                result[key] = _scalar(value)
                i += 1
            elif i + 1 < len(lines) and lines[i + 1][0] > indent:
                result[key], i = block(i + 1, lines[i + 1][0])
            elif i + 1 < len(lines) and lines[i + 1][0] == indent and lines[i + 1][1].startswith('- '):
                result[key], i = block(i + 1, indent)
            else:
                result[key] = ''
                i += 1
        if i < len(lines) and lines[i][0] > indent:
            raise ValueError('platforms.yml:%d: unexpected indent' % lines[i][2])
        return result, i

    def sequence(i, indent):
        result = []
        while i < len(lines) and lines[i][0] == indent and lines[i][1].startswith('- '):
            rest = lines[i][1][2:].strip()
            if ':' not in rest:
                result.append(_scalar(rest))
                i += 1
                continue
            # "- key: value" opens a mapping, its other keys two spaces further
            lines[i] = [indent + 2, rest, lines[i][2]]
            item, i = mapping(i, indent + 2)
            result.append(item)
        return result, i

    if not lines:
        return {}
    data, i = block(0, lines[0][0])
    if i != len(lines):
        raise ValueError('platforms.yml:%d: cannot be read' % lines[i][2])
    return data


def load(path=SOURCE):
    """platforms.yml, checked, with the links of the downloads filled in"""
    with open(path, encoding='utf-8') as f:
        data = parse_yaml(f.read())
    repository = data.get('repository', 'KuchiSofts/vlc-redesign')
    seen = set()
    for p in data.get('platforms', []):
        for key in ('id', 'name', 'status'):
            if not p.get(key):
                raise ValueError('platforms.yml: a platform has no %s' % key)
        if p['id'] in seen:
            raise ValueError('platforms.yml: %s twice' % p['id'])
        seen.add(p['id'])
        if p['status'] not in STATUS:
            raise ValueError('platforms.yml: %s: unknown status %s' % (p['id'], p['status']))
        if p['status'] in AVAILABLE and not p.get('version'):
            raise ValueError('platforms.yml: %s is %s but has no version' % (p['id'], p['status']))
        if p.get('date'):
            datetime.date.fromisoformat(p['date'])
        p.setdefault('short', p['name'])
        downloads = p.get('downloads') or []
        if not isinstance(downloads, list):
            raise ValueError('platforms.yml: %s: downloads must be a list' % p['id'])
        for d in downloads:
            d['file'] = d['file'].replace('{version}', p.get('version', ''))
            d['url'] = 'https://github.com/%s/releases/download/v%s/%s' % (
                p.get('repository', repository), p.get('version', ''), d['file'])
        p['downloads'] = downloads if p['status'] in AVAILABLE else []
    data['repository'] = repository
    return data


def bar_text(p):
    """Its word in the platform bar: "Windows 0.3.1", "Linux soon" """
    if p['status'] in AVAILABLE:
        return '%s %s%s' % (p['short'], p['version'], ' beta' if p['status'] == 'beta' else '')
    return '%s %s' % (p['short'], STATUS[p['status']][2])


def long_date(iso):
    d = datetime.date.fromisoformat(iso)
    return '%d %s %d' % (d.day, MONTHS[d.month - 1], d.year)


MONTHS = ['January', 'February', 'March', 'April', 'May', 'June', 'July', 'August', 'September',
          'October', 'November', 'December']


# --- README ----------------------------------------------------------------

def readme_block(data):
    repo = 'https://github.com/' + data['repository']
    bar = []
    for p in data['platforms']:
        text = bar_text(p)
        if p['status'] in AVAILABLE:
            bar.append('**[%s](%s)**' % (text, p.get('page') or repo + '/releases/latest'))
        else:
            bar.append(text)
    rows = ['| Platform | Status | Latest | Get it |', '|---|---|---|---|']
    for p in data['platforms']:
        label, mark, _ = STATUS[p['status']]
        latest = p.get('version') if p['status'] in AVAILABLE else '—'
        if p['status'] in AVAILABLE and p.get('date'):
            latest += ' · ' + p['date']
        if p['downloads']:
            get = ' · '.join('[%s](%s)' % (d['label'].lower(), d['url']) for d in p['downloads'])
        else:
            get = 'coming soon'
        rows.append('| %s | %s %s | %s | %s |' % (p['short'], mark, label, latest, get))
    site = data.get('site')
    more = (' · [every platform on the website](%s)' % site) if site else ''
    return ('<!-- platforms:start -->\n'
            '<!-- Written by extras/juxtopposed/platforms/render.py from platforms.yml: edit that file. -->\n'
            '<p align="center"><sub>\n\n'
            '%s\n\n'
            '</sub></p>\n\n'
            '<details>\n<summary>Every platform and its status</summary>\n\n'
            '%s\n\n'
            '<sub>From [`platforms.yml`](platforms.yml)%s.</sub>\n\n'
            '</details>\n'
            '<!-- platforms:end -->') % (' · '.join(bar), '\n'.join(rows), more)


def replace_block(text, block, start='<!-- platforms:start -->', end='<!-- platforms:end -->'):
    a = text.find(start)
    b = text.find(end)
    if a < 0 or b < a:
        raise ValueError('the markers %s and %s are missing' % (start, end))
    # the block carries its own markers
    block_start = block.find(start)
    inner = block[block_start:] if block_start >= 0 else start + block + end
    # (in the line ends of the file: README.md is kept with CRLF)
    if '\r\n' in text:
        inner = inner.replace('\r\n', '\n').replace('\n', '\r\n')
    return text[:a] + inner + text[b + len(end):]


# --- "Also on" -------------------------------------------------------------

def also_on(data, exclude, markdown=True):
    """ "Also on: Linux soon · macOS soon · …" without the platform `exclude`
    (the release notes of Windows leave Windows out)"""
    items = []
    for p in data['platforms']:
        if p['id'] == exclude:
            continue
        text = bar_text(p)
        if markdown and p['status'] in AVAILABLE and p.get('page'):
            text = '[%s](%s)' % (text, p['page'])
        items.append(text)
    if not items:
        return ''
    line = ' · '.join(items)
    if not markdown:
        return 'Also on: ' + line
    site = data.get('site')
    return '**Also on:** %s%s' % (line, (' · [every platform](%s)' % site) if site else '')


# --- the website -----------------------------------------------------------

def site_json(data):
    out = {
        '_comment': 'Written by extras/juxtopposed/platforms/render.py from platforms.yml: edit that file.',
        'repository': data['repository'],
        'site': data.get('site', ''),
        'platforms': [],
    }
    for p in data['platforms']:
        label, _, _ = STATUS[p['status']]
        item = {
            'id': p['id'],
            'name': p['name'],
            'short': p['short'],
            'status': p['status'],
            'statusLabel': label,
            'available': p['status'] in AVAILABLE,
            'bar': bar_text(p),
        }
        for key in ('version', 'date', 'next', 'os', 'requires', 'notes', 'page', 'repository'):
            if p.get(key):
                item[key] = p[key]
        item['downloads'] = [{'label': d['label'], 'file': d['file'], 'url': d['url']} for d in p['downloads']]
        out['platforms'].append(item)
    return json.dumps(out, indent=2, ensure_ascii=False) + '\n'


def _e(text):
    return html.escape(text, quote=True)


def site_platforms_block(data):
    items = []
    for p in data['platforms']:
        label = STATUS[p['status']][0]
        if p['status'] in AVAILABLE:
            latest = 'Version %s' % p['version']
            if p.get('date'):
                latest += ' · <time datetime="%s">%s</time>' % (_e(p['date']), long_date(p['date']))
            links = ''.join('<a class="button small%s" href="%s">%s</a>'
                            % (' accent' if n == 0 else '', _e(d['url']), _e(d['label']))
                            for n, d in enumerate(p['downloads']))
        else:
            latest = 'Next: %s' % _e(p['next']) if p.get('next') else 'Coming %s' % (
                'later' if p['status'] == 'later' else 'soon')
            links = '<a class="link" href="%s">Follow the progress</a>' % _e(p.get('page', '#')) if p.get('page') else ''
        items.append(
            '        <li class="platform" id="platform-%(id)s" data-os="%(os)s">\n'
            '          <div class="platform-head"><h3>%(name)s</h3>'
            '<span class="status status-%(status)s">%(label)s</span></div>\n'
            '          <p class="latest">%(latest)s</p>\n'
            '          <p class="requires">%(requires)s</p>\n'
            '          <p class="notes">%(notes)s</p>\n'
            '          <div class="actions">%(links)s</div>\n'
            '        </li>' % {
                'id': _e(p['id']), 'os': _e(p.get('os', '')), 'name': _e(p['name']), 'status': _e(p['status']),
                'label': _e(label), 'latest': latest, 'requires': _e(p.get('requires', '')),
                'notes': _e(p.get('notes', '')), 'links': links})
    return ('<!-- platforms:start -->\n'
            '        <!-- Written by render.py from platforms.yml -->\n'
            + '\n'.join(items) +
            '\n        <!-- platforms:end -->')


def site_download_block(data):
    """The download box without JavaScript: the first released platform"""
    p = next((p for p in data['platforms'] if p['status'] in AVAILABLE and p['downloads']), None)
    if not p:
        return '<!-- download:start --><!-- download:end -->'
    first, rest = p['downloads'][0], p['downloads'][1:]
    meta = 'Version %s' % _e(p['version'])
    if p.get('date'):
        meta += ' · <time datetime="%s">%s</time>' % (_e(p['date']), long_date(p['date']))
    if p.get('requires'):
        meta += ' · ' + _e(p['requires'])
    return ('<!-- download:start -->\n'
            '          <p class="download-title" id="download-title">VLC Redesign for %s</p>\n'
            '          <div class="download-buttons" id="download-buttons">'
            '<a class="button accent large" href="%s">Download for %s</a>'
            '%s</div>\n'
            '          <p class="download-meta" id="download-meta">%s</p>\n'
            '          <!-- download:end -->') % (
        _e(p['name']), _e(first['url']), _e(p['short']),
        ''.join('<a class="button large" href="%s">%s</a>' % (_e(d['url']), _e(d['label'])) for d in rest),
        meta)


# --- writing ---------------------------------------------------------------

def outputs(data):
    """{path: the text it should have}"""
    result = {}
    with open(README, encoding='utf-8', newline='') as f:
        result[README] = replace_block(f.read(), readme_block(data))
    result[SITE_JSON] = site_json(data)
    if os.path.exists(SITE_INDEX):
        with open(SITE_INDEX, encoding='utf-8', newline='') as f:
            page = f.read()
        page = replace_block(page, site_platforms_block(data))
        page = replace_block(page, site_download_block(data), '<!-- download:start -->', '<!-- download:end -->')
        result[SITE_INDEX] = page
    return result


def current(path):
    try:
        with open(path, encoding='utf-8', newline='') as f:
            return f.read()
    except FileNotFoundError:
        return None


def build(target, data):
    """The website as GitHub Pages serves it"""
    # emptied, not removed: a local server may be serving from it
    if os.path.isdir(target):
        for name in os.listdir(target):
            path = os.path.join(target, name)
            if os.path.isdir(path):
                shutil.rmtree(path)
            else:
                os.remove(path)
    shutil.copytree(SITE, target, dirs_exist_ok=True)
    media = os.path.join(target, 'media')
    os.makedirs(media, exist_ok=True)
    for rel in SCREENSHOTS:
        src = os.path.join(ROOT, rel)
        if os.path.exists(src):
            shutil.copy2(src, os.path.join(media, os.path.basename(rel)))
    # no Jekyll on GitHub Pages: the files as they are
    open(os.path.join(target, '.nojekyll'), 'w').close()


def main(argv):
    data = load()
    if '--also-on' in argv:
        i = argv.index('--also-on')
        print(also_on(data, argv[i + 1] if i + 1 < len(argv) else ''))
        return 0
    wanted = outputs(data)
    if '--check' in argv:
        stale = [os.path.relpath(p, ROOT) for p, text in wanted.items() if current(p) != text]
        if stale:
            print('Out of date (run extras/juxtopposed/platforms/render.py): ' + ', '.join(stale))
            return 1
        print('Up to date')
        return 0
    for path, text in wanted.items():
        if current(path) != text:
            os.makedirs(os.path.dirname(path), exist_ok=True)
            with open(path, 'w', encoding='utf-8', newline='') as f:
                f.write(text)
            print('wrote', os.path.relpath(path, ROOT))
    if '--build' in argv:
        i = argv.index('--build')
        target = os.path.abspath(argv[i + 1])
        build(target, data)
        print('website in', target)
    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
