"""Checks that an upload leaves nothing of the project behind: the version, the
changelog, the roadmap, the plan, the README, the platforms and the website,
the wiki, the translations, VideoLAN's master, the credits and every issue.

  python release-check.py                 an upload of work in progress (the
                                          version being prepared, not released)
  python release-check.py --release X.Y.Z the release of X.Y.Z (its tag)
  options: --base <ref>   what is already uploaded (default github/main)
           --offline      no GitHub or VideoLAN requests (the issue and
                          upstream checks are then reported as not done)

Exit 0 when every check passes, 1 otherwise. The pre-push hook runs it for
every push to the public repository and make-release.ps1 before packaging;
docs/RELEASE-CHECKLIST.md lists what a person does around it."""
import json, os, re, subprocess, sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))
REPO = 'KuchiSofts/vlc-redesign'
UPSTREAM = 'origin/master'

args = sys.argv[1:]
release = args[args.index('--release') + 1] if '--release' in args else None
base = args[args.index('--base') + 1] if '--base' in args else 'github/main'
offline = '--offline' in args

results = []  # (ok, name, detail)
todo = []     # what a person does after the push


def check(ok, name, detail=''):
    results.append((bool(ok), name, detail))


def git(*a):
    r = subprocess.run(['git', '-C', ROOT] + list(a), capture_output=True, text=True, encoding='utf-8')
    return r.stdout.strip() if r.returncode == 0 else None


def read(path):
    try:
        with open(os.path.join(ROOT, path), encoding='utf-8') as f:
            return f.read()
    except OSError:
        return ''


def gh(*a):
    if offline:
        return None
    try:
        r = subprocess.run(['gh'] + list(a), capture_output=True, text=True, encoding='utf-8', timeout=60)
    except (OSError, subprocess.TimeoutExpired):
        return None
    return r.stdout if r.returncode == 0 else None


def vkey(v):
    return tuple(int(x) for x in v.split('.'))


# --- the version ------------------------------------------------------------
m = re.search(r'#define VLC_REDESIGN_VERSION "([0-9.]+)"', read('modules/gui/qt/util/redesign_version.hpp'))
version = m.group(1) if m else '?'
# the branch of a release goes up before its tag: once the changelog dates
# the version, an upload is its release
if not release and re.search(r'(?m)^## \[%s\] · \d{4}-\d{2}-\d{2}' % re.escape(version), read('CHANGELOG.md')):
    release = version
if release:
    check(version == release, 'Version in the app',
          'redesign_version.hpp says %s, the release is %s' % (version, release))
target = release or version

# --- what this upload changes -------------------------------------------------
if git('rev-parse', '--verify', '-q', base) is None:
    changed = []
    check(False, 'What is already uploaded', '%s is unknown: git fetch github' % base)
else:
    changed = (git('diff', '--name-only', base, 'HEAD') or '').splitlines()
docs_only = re.compile(r'^(docs/|site/|design/build/|po/|\.github/)|\.md$|^platforms\.yml$')
code_changed = [f for f in changed if not docs_only.search(f)]
check(git('status', '--porcelain', '--untracked-files=no') == '', 'Everything committed',
      'uncommitted changes to tracked files')

# --- the changelog ------------------------------------------------------------
changelog = read('CHANGELOG.md')
sections = re.split(r'(?m)^## ', changelog)
section = {}
for s in sections[1:]:
    head = s.split('\n', 1)[0]
    mm = re.match(r'\[([^\]]+)\]', head)
    if mm:
        section[mm.group(1)] = (head, s.split('\n', 1)[1] if '\n' in s else '')
entries = lambda body: [l for l in body.splitlines() if l.startswith('- ')]
if release:
    head, body = section.get(release, ('', ''))
    dated = re.search(r'\d{4}-\d{2}-\d{2}', head)
    check(head and dated, 'CHANGELOG: a dated section for %s' % release, '"## [%s] · YYYY-MM-DD" missing' % release)
    check(entries(body), 'CHANGELOG: what %s changes' % release, 'the section is empty')
    anchor = '#' + re.sub(r'[^a-z0-9 -]', '', head.lower()).replace(' ', '-')
    check(anchor in changelog.split('\n## ', 1)[0], 'CHANGELOG: %s in the contents line' % release,
          'no link %s in **Contents:**' % anchor)
    check(not entries(section.get('Unreleased', ('', ''))[1]), 'CHANGELOG: nothing left in Unreleased',
          'entries under [Unreleased] belong to %s or to the next version' % release)
    body_of_target = body
else:
    body_of_target = section.get('Unreleased', ('', ''))[1]
    if code_changed:
        check('CHANGELOG.md' in changed and entries(body_of_target), 'CHANGELOG: the changes of this upload',
              '%d code files changed, CHANGELOG.md [Unreleased] not updated' % len(code_changed))

# --- the documents every upload updates --------------------------------------
if code_changed or release:
    for path, why in (('ROADMAP.md', 'what is done and what is next'),
                      ('docs/MASTER-PLAN.md', 'the Now section and the dashboard'),
                      ('README.md', 'the platform bar, the features')):
        if path == 'README.md' and not release:
            continue
        check(path in changed, '%s updated' % path, '%s (%s) is not in this upload' % (path, why))
for path in ('README.md', 'ROADMAP.md', 'docs/MASTER-PLAN.md'):
    check(target in read(path), '%s names %s' % (path, target), 'no mention of %s' % target)
if release:
    for path in ('docs/wiki/Install-and-Update.md', 'docs/wiki/Home.md'):
        check(release in read(path), '%s names %s' % (path, release), 'the wiki still speaks of an older version')

# --- platforms, website, design tokens ----------------------------------------
platforms = read('platforms.yml')
win = re.search(r'(?ms)^  - id: windows.*?(?=^  - id: |\Z)', platforms)
win = win.group(0) if win else ''
if release:
    check(re.search(r'(?m)^\s+version: %s\s*$' % re.escape(release), win), 'platforms.yml: Windows at %s' % release,
          'set version: %s, date: and next:' % release)
    nxt = re.search(r'(?m)^\s+next: ([0-9.]+)', win)
    check(nxt and vkey(nxt.group(1)) > vkey(release), 'platforms.yml: the next version', 'next: must be after %s' % release)
else:
    nxt = re.search(r'(?m)^\s+next: ([0-9.]+)', win)
    check(nxt and nxt.group(1) == version, 'platforms.yml: next is %s' % version, 'next: should be the version being prepared')
r = subprocess.run([sys.executable, os.path.join(ROOT, 'extras/juxtopposed/platforms/render.py'), '--check'],
                   capture_output=True, text=True)
check(r.returncode == 0, 'README platform bar and website in step with platforms.yml',
      'run extras/juxtopposed/platforms/render.py')
r = subprocess.run([sys.executable, os.path.join(ROOT, 'design/tools/check_qml.py')], capture_output=True, text=True)
check(r.returncode == 0, 'Design tokens match the app', (r.stdout + r.stderr).strip().splitlines()[-1:] or '')

# the wiki's Roadmap page carries the status table and the next steps of
# ROADMAP.md, word for word: the wiki is where most people look
def roadmap_part(text):
    i = text.find('## Where each program stands')
    j = text.find('\n## ', text.find('## Next', i)) if i >= 0 else -1
    return text[i:j].strip() if i >= 0 and j > i else None
try:
    part = roadmap_part(open(os.path.join(ROOT, 'ROADMAP.md'), encoding='utf-8').read())
    wiki = open(os.path.join(ROOT, 'docs/wiki/Roadmap.md'), encoding='utf-8').read()
except OSError:
    part, wiki = None, ''
check(part is not None and part in wiki, 'docs/wiki/Roadmap.md in step with ROADMAP.md',
      'copy the "Where each program stands" and "Next" sections of ROADMAP.md into docs/wiki/Roadmap.md')

# --- translations ---------------------------------------------------------------
t_text = git('log', '-1', '--format=%ct', '-G', r'qsTr\(|qtr\(|_\("|N_\("', '--', 'modules/gui/qt')
t_pot = git('log', '-1', '--format=%ct', '--', 'po/vlc.pot')
check(t_text and t_pot and int(t_pot) >= int(t_text), 'Translations: po/vlc.pot has every new text',
      'texts changed after the last po/vlc.pot: potgen, msgmerge, translate, weblate-sync.py export')
# a file of the redesign with texts that po/POTFILES.in does not list never
# reaches vlc.pot: its texts stay English in every language
listed = set(l.strip() for l in read('po/POTFILES.in').splitlines() if l.strip() and not l.startswith('#'))
ours = set((git('diff', '--name-only', '%s...HEAD' % UPSTREAM) or '').split())
with_texts = set((git('grep', '-l', '-E', r'qsTr\(|qtr\(|\bN?_\("|NC_\(', '--',
                      'modules/gui/qt/*.cpp', 'modules/gui/qt/*.hpp', 'modules/gui/qt/*.qml',
                      'modules/misc/iptv/*.cpp') or '').split())
unlisted = sorted(f for f in with_texts & ours - listed if '/test' not in f)
check(not unlisted, 'Translations: po/POTFILES.in lists every file of the redesign with texts',
      'add to po/POTFILES.in: ' + ', '.join(unlisted[:8]) + (' ...' if len(unlisted) > 8 else ''))

# --- VideoLAN -----------------------------------------------------------------------
if not offline:
    subprocess.run(['git', '-C', ROOT, 'fetch', '-q', 'origin', 'master'], capture_output=True, timeout=300)
behind = git('rev-list', '--count', 'HEAD..' + UPSTREAM)
if behind is None:
    check(False, "VideoLAN's master", 'cannot read %s' % UPSTREAM)
else:
    head_up = git('rev-parse', '--short=10', UPSTREAM)
    check(behind == '0', "VideoLAN's master merged", '%s commits behind %s: merge, build, test' % (behind, head_up))
    if behind == '0':
        merged = [git('rev-parse', '--short=10', UPSTREAM), git('rev-parse', '--short', UPSTREAM)]
        check(any(h and h in read('CHANGELOG.md') + read('docs/MASTER-PLAN.md') for h in merged),
              'The merged VideoLAN commit is written down', 'name %s in the changelog or the plan' % head_up)

# --- issues and credits ----------------------------------------------------------
roadmap = read('ROADMAP.md') + read('docs/MASTER-PLAN.md')
credits = read('CREDITS.md')
issues = gh('issue', 'list', '-R', REPO, '--state', 'all', '--limit', '500', '--json', 'number,state,title,author')
if issues is None:
    check(False, 'Issues', 'GitHub not reachable (gh) or --offline: every issue unchecked')
else:
    issues = json.loads(issues)
    mentioned = lambda n, text: re.search(r'(#|issues/)%d\b' % n, text)
    for i in issues:
        n, who = i['number'], i['author']['login']
        if i['state'] == 'OPEN':
            check(mentioned(n, body_of_target) or mentioned(n, roadmap), 'Issue #%d accounted for' % n,
                  '"%s": not in the changelog of %s, nor in the roadmap or the plan' % (i['title'], target))
            if release and mentioned(n, body_of_target):
                todo.append('Reply on #%d (what was fixed, in %s, how to update) and close it' % (n, release))
        if not who.startswith('app/') and (mentioned(n, body_of_target) or i['state'] == 'CLOSED'):
            check(('@' + who) in credits, 'Credit for @%s (#%d)' % (who, n), 'add them to CREDITS.md')

# --- the designer's credit ------------------------------------------------------------
# Juxtopposed designed every screen (CC BY 4.0): named in every version and
# every place a user meets the program
for path in ('modules/gui/qt/dialogs/dialogs/qml/RedesignAboutDialog.qml', 'README.md', 'site/index.html',
             'CREDITS.md', 'docs/wiki/Credits-and-License.md'):
    check('Juxtopposed' in read(path), 'Designer credited: %s' % path, 'Juxtopposed is not named')
android = os.path.join(os.path.dirname(os.path.dirname(ROOT)), 'vlc-android-redesign')
if os.path.isdir(android):
    # a text the app shows (strings.xml), not a comment in the code
    found = subprocess.run(['git', '-C', android, 'grep', '-l', 'Juxtopposed', '--', 'application/*strings*.xml'],
                           capture_output=True, text=True).stdout.strip()
    check(found, 'Designer credited: the Android app', 'Juxtopposed is not named in the Android app')
if release:
    todo.append('The release notes start with the Juxtopposed credit header (release-notes.py)')

# --- outside the repository --------------------------------------------------------
private = os.path.join(os.path.dirname(os.path.dirname(ROOT)), 'VLC-Redesign-Roadmap.md')
if os.path.exists(private):
    with open(private, encoding='utf-8') as f:
        text = f.read()
    check(target in text, 'The owner\'s detailed roadmap (outside the repository) names %s' % target, private)

if release:
    todo += ['GitHub release v%s: setup, zip, every .vrdelta, SHA256SUMS.txt and SHA256SUMS.txt.sig' % release,
             'Announce %s in Discussions › Announcements' % release,
             'Weblate: weblate-sync.py export --upstream <merged VideoLAN commit>',
             'Check the website and the README on GitHub show %s' % release]

# --- report -------------------------------------------------------------------------
failed = [r for r in results if not r[0]]
print('Release check: %s (%d checks, %d to fix)' % ('release ' + release if release else 'upload of ' + version,
                                                     len(results), len(failed)))
for ok, name, detail in results:
    print('  %s %s%s' % ('ok ' if ok else 'FIX', name, '' if ok else ': ' + str(detail)))
if todo:
    print('After the push:')
    for t in todo:
        print('  - ' + t)
sys.exit(1 if failed else 0)
