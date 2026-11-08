#!/usr/bin/env python3
# Copyright (C) 2026 VLC authors and VideoLAN
# SPDX-License-Identifier: GPL-2.0-or-later
"""The catalog's lists of iptv-org (https://github.com/iptv-org/iptv), made from its API
(https://iptv-org.github.io/api/): one entry for every country and every category that has channels, with the
catalog's guides of that country; and the last check of every list and guide of the catalog (the copy the app
fetches once a day from the website, published daily by .github/workflows/platforms.yml): "works" (a list with its
channels counted), or, failing twice in a row, "not answering" (sorted last, not removed).

  python modules/misc/iptv/catalog/iptv_org.py [--no-check] [--no-api] [--previous FILE]

--previous: the published catalog, whose failures in a row carry on (the daily check of the website does not commit
its catalog). --no-api: the checks alone. Writes catalog/catalog.json (LF). Only free, legal, public lists: iptv-org
lists publicly available streams."""
import collections
import concurrent.futures
import datetime
import json
import os
import sys
import urllib.request

HERE = os.path.dirname(os.path.abspath(__file__))
CATALOG = os.path.join(HERE, 'catalog.json')
API = 'https://iptv-org.github.io/api/'
LISTS = 'https://iptv-org.github.io/iptv/'
AGENT = {'User-Agent': 'VLC-Redesign catalog (https://github.com/KuchiSofts/vlc-redesign)'}
# ISO 639-3 (iptv-org's) to the 639-1 the catalog and the app use
LANGUAGES = {
    'eng': 'en', 'spa': 'es', 'fra': 'fr', 'deu': 'de', 'ita': 'it', 'por': 'pt', 'rus': 'ru', 'ara': 'ar',
    'heb': 'he', 'tur': 'tr', 'pol': 'pl', 'nld': 'nl', 'ukr': 'uk', 'ron': 'ro', 'hun': 'hu', 'ces': 'cs',
    'slk': 'sk', 'ell': 'el', 'bul': 'bg', 'srp': 'sr', 'hrv': 'hr', 'bos': 'bs', 'slv': 'sl', 'mkd': 'mk',
    'sqi': 'sq', 'swe': 'sv', 'nor': 'no', 'nob': 'no', 'dan': 'da', 'fin': 'fi', 'est': 'et', 'lav': 'lv',
    'lit': 'lt', 'isl': 'is', 'gle': 'ga', 'cym': 'cy', 'cat': 'ca', 'eus': 'eu', 'glg': 'gl', 'fas': 'fa',
    'urd': 'ur', 'hin': 'hi', 'ben': 'bn', 'pan': 'pa', 'guj': 'gu', 'tam': 'ta', 'tel': 'te', 'kan': 'kn',
    'mal': 'ml', 'mar': 'mr', 'nep': 'ne', 'sin': 'si', 'tha': 'th', 'vie': 'vi', 'ind': 'id', 'msa': 'ms',
    'zsm': 'ms', 'fil': 'tl', 'tgl': 'tl', 'zho': 'zh', 'cmn': 'zh', 'yue': 'zh', 'jpn': 'ja', 'kor': 'ko',
    'kat': 'ka', 'hye': 'hy', 'aze': 'az', 'kaz': 'kk', 'uzb': 'uz', 'kir': 'ky', 'tgk': 'tg', 'tuk': 'tk',
    'mon': 'mn', 'amh': 'am', 'som': 'so', 'swa': 'sw', 'hau': 'ha', 'yor': 'yo', 'ibo': 'ig', 'zul': 'zu',
    'afr': 'af', 'mlt': 'mt', 'bel': 'be', 'khm': 'km', 'lao': 'lo', 'mya': 'my', 'pus': 'ps', 'kur': 'ku',
    'ckb': 'ku', 'que': 'qu', 'grn': 'gn', 'hat': 'ht', 'ltz': 'lb', 'fao': 'fo', 'kal': 'kl',
}


def get(url, timeout=60):
    with urllib.request.urlopen(urllib.request.Request(url, headers=AGENT), timeout=timeout) as r:
        return r.read()


def channels_in(url):
    """the channels of a list, or None when it doesn't answer"""
    try:
        text = get(url, 30).decode('utf-8', 'replace')
    except Exception:
        return None
    return sum(1 for line in text.splitlines() if line.startswith('#EXTINF'))


def guide_answers(url):
    """whether a guide answers with a guide: its first bytes only (some are hundreds of MB)"""
    try:
        request = urllib.request.Request(url, headers=dict(AGENT, Range='bytes=0-4095'))
        with urllib.request.urlopen(request, timeout=30) as r:
            head = r.read(4096)
    except Exception:
        return False
    # gzip, or XML
    return head[:2] == b'\x1f\x8b' or b'<tv' in head or b'<?xml' in head


def country_of(code):
    """iptv-org's country code as the catalog's (ISO: it names the United Kingdom "UK")"""
    return 'GB' if code == 'UK' else code


def main():
    check = '--no-check' not in sys.argv
    api = '--no-api' not in sys.argv
    catalog = json.load(open(CATALOG, encoding='utf-8'))
    sources = catalog['sources']
    by_id = {s['id']: s for s in sources}
    if '--previous' in sys.argv:
        try:
            previous = json.load(open(sys.argv[sys.argv.index('--previous') + 1], encoding='utf-8'))
            for old in previous.get('sources', []):
                s = by_id.get(old.get('id'))
                if s is not None and old.get('url') == s.get('url'):
                    for key in ('failures', 'status', 'verified'):
                        if key in old:
                            s[key] = old[key]
        except (OSError, ValueError, IndexError):
            pass
    if api:
        add_iptv_org(catalog, sources, by_id)
    # the last check of every list and guide of the catalog: "works" (a list
    # with its channels), "not answering" after two failures in a row
    if check:
        today = datetime.date.today().isoformat()
        lists = [s for s in sources if s['kind'] in ('m3u', 'epg') and s['url'].startswith('http')]

        def checked(s):
            return channels_in(s['url']) if s['kind'] == 'm3u' else (1 if guide_answers(s['url']) else None)

        with concurrent.futures.ThreadPoolExecutor(8) as pool:
            counts = list(pool.map(checked, lists))
        for s, count in zip(lists, counts):
            if count is None or count == 0:
                s['failures'] = s.get('failures', 0) + 1
                s['status'] = 'not answering' if s['failures'] >= 2 else s.get('status') or 'works'
            else:
                s['failures'] = 0
                s['status'] = 'works'
                if s['kind'] == 'm3u':
                    s['channels'] = count
                s['verified'] = today
    catalog['updated'] = datetime.date.today().isoformat()
    with open(CATALOG, 'w', encoding='utf-8', newline='\n') as f:
        json.dump(catalog, f, ensure_ascii=False, indent=1)
        f.write('\n')
    print('%d sources, %d lists of iptv-org countries, %d of categories, %d not answering' % (
        len(sources), sum(1 for s in sources if s['id'].startswith('iptv-org-country')),
        sum(1 for s in sources if s['id'].startswith('iptv-org-cat')),
        sum(1 for s in sources if s.get('status') == 'not answering')))


def add_iptv_org(catalog, sources, by_id):
    """one entry for every country and category of iptv-org that has channels"""
    countries = json.loads(get(API + 'countries.json'))
    categories = json.loads(get(API + 'categories.json'))
    channels = {c['id']: c for c in json.loads(get(API + 'channels.json'))}
    streams = json.loads(get(API + 'streams.json'))
    per_country = collections.Counter()
    per_category = collections.Counter()
    categories_of_country = collections.defaultdict(collections.Counter)
    for s in streams:
        c = channels.get(s.get('channel') or '')
        if not c or c.get('is_nsfw'):
            continue
        per_country[country_of(c['country'])] += 1
        for cat in c.get('categories') or []:
            per_category[cat] += 1
            categories_of_country[country_of(c['country'])][cat] += 1
    # the catalog's guides of a country (not the big ones of every country)
    guides = collections.defaultdict(list)
    for g in sources:
        if g['kind'] == 'epg':
            for cc in g.get('countries') or []:
                guides[cc].append(g['url'])
    # iptv-epg.org first: its guides name the channels by iptv-org's ids, as
    # iptv-org's lists do (tvg-id)
    preferred = ('iptv-epg.org', 'epgshare01.online', 'open-epg.com', 'epg.pw')

    def rank(url):
        host = url.split('/')[2]
        return next((i for i, h in enumerate(preferred) if host.endswith(h)), len(preferred))

    for cc in guides:
        guides[cc].sort(key=rank)

    def entry(eid, name, url, description, languages, ccs, cats, count, epg):
        s = by_id.get(eid)
        if s is None:
            s = {'id': eid}
            sources.append(s)
            by_id[eid] = s
        s.update({
            'name': name, 'kind': 'm3u', 'url': url,
            'epg': epg[:2] if epg else s.get('epg') or [],
            'website': 'https://github.com/iptv-org/iptv', 'description': description,
            'languages': languages, 'countries': ccs, 'categories': cats, 'channels': count,
            'catchup': {'supported': False, 'days': 0}, 'vod': False, 'series': False, 'price': 'free',
            'signup': False, 'logo': 'https://github.com/iptv-org.png',
        })
        s.setdefault('notes', 'Community-maintained list of publicly available streams; some channels are geo-blocked '
                              'or not 24/7 (marked in the channel name).')

    for country in countries:
        # (the list keeps iptv-org's code in its address: countries/uk.m3u)
        org = country['code']
        cc = country_of(org)
        if per_country[cc] == 0:
            continue
        languages = []
        for lang in country.get('languages') or []:
            code = LANGUAGES.get(lang, lang)
            if code not in languages:
                languages.append(code)
        cats = [c for c, _ in categories_of_country[cc].most_common(7)]
        entry('iptv-org-country-' + org.lower(), 'iptv-org: ' + country['name'], LISTS + 'countries/%s.m3u' % org.lower(),
              'Publicly available channels from %s.' % country['name'], languages[:3], [cc], cats, per_country[cc],
              guides.get(cc, []))
    for cat in categories:
        cid = cat['id']
        if per_category[cid] == 0 or cid == 'xxx':
            continue
        entry('iptv-org-cat-' + cid, 'iptv-org: %s channels' % cat['name'], LISTS + 'categories/%s.m3u' % cid,
              'Publicly available %s channels from around the world.' % cat['name'].lower(), [], [], [cid],
              per_category[cid], [])


if __name__ == '__main__':
    main()
