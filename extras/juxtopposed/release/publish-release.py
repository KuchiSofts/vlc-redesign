# Copyright (C) 2026 VLC authors and VideoLAN
# SPDX-License-Identifier: GPL-2.0-or-later
"""A release from a pull request whose checks have passed, in one command (the release PC: Windows, gh, Python with
bsdiff4, Inno Setup, the release key). What was some forty steps by hand, each one a place for a slip:

  python publish-release.py --pr N [--apk FILE] [--tv-zip FILE] [--work DIR] [--dist DIR] [--full] [--publish]

 1. the pull request: this commit is its head, every check passed, the commit GitHub built (its merge with main)
    has this commit's tree: what was built and tested is what is tagged;
 2. its builds: the Windows packages (x64, ARM64), the Linux AppImages and folders, the macOS disk images, each
    checked against the checksum built with it;
 3. the Windows files: make-release.ps1 for x64 then ARM64 (the installer, the portable zip, ONE small update from
    the release before, checked by applying it), ci/pe-check.py on both;
 4. everything in --dist with --apk (the signed APK) and --tv-zip (the Samsung TV app, when it changed);
    SHA256SUMS.txt of all of it, signed with the release key and verified with the key the app carries;
 5. the notes (release-notes.py).
Without --publish it stops here: the files and the notes are ready to look at. With --publish:
 6. the tag vX.Y.Z on this commit, pushed; the pull request merged;
 7. the GitHub release as a draft with every file, each one's digest on GitHub compared with the signed list;
 8. published as the latest; SHA256SUMS.txt and its signature downloaded again and verified.

Each step says what it did; a step already done (a file there, a tag there) is not done twice, so a run that
stopped is run again. --full: a full release (make-release.ps1 without -Update). The Android repository's own tag
and pull request stay by hand: another repository."""
import argparse, hashlib, json, os, re, shutil, subprocess, sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(os.path.dirname(HERE)))
REPO = 'KuchiSofts/vlc-redesign'
HOME = os.path.expanduser('~')

parser = argparse.ArgumentParser(description=__doc__.split('\n')[0])
parser.add_argument('--pr', type=int, required=True)
parser.add_argument('--apk')
parser.add_argument('--tv-zip')
parser.add_argument('--work', default=os.path.join(HOME, 'source', 'vlc-redesign-ci'))
parser.add_argument('--dist', default=os.path.join(HOME, 'source', 'vlc-redesign-dist'))
parser.add_argument('--full', action='store_true')
parser.add_argument('--publish', action='store_true')
args = parser.parse_args()


def run(cmd, check=True, cwd=None, quiet=False):
    r = subprocess.run(cmd, capture_output=True, text=True, encoding='utf-8', errors='replace', cwd=cwd)
    if check and r.returncode != 0:
        sys.exit('FAILED: %s\n%s%s' % (' '.join(cmd), r.stdout[-2000:], r.stderr[-2000:]))
    return r if quiet else r.stdout.strip()


def git(*a):
    return run(['git', '-C', ROOT] + list(a))


def gh(*a, check=True):
    return run(['gh'] + list(a) + ['--repo', REPO], check=check)


def api(path):
    return json.loads(run(['gh', 'api', path]))


def sha256(path):
    h = hashlib.sha256()
    with open(path, 'rb') as f:
        for block in iter(lambda: f.read(1 << 20), b''):
            h.update(block)
    return h.hexdigest()


def step(text):
    print('ok   ' + text, flush=True)


def powershell(script, *a):
    r = subprocess.run(['powershell', '-NoProfile', '-ExecutionPolicy', 'Bypass', '-File', script] + list(a), cwd=ROOT)
    if r.returncode != 0:
        sys.exit('FAILED: %s %s (%d)' % (os.path.basename(script), ' '.join(a), r.returncode))


# --- 1. the pull request -------------------------------------------------------------------------------------
version = re.search(r'VLC_REDESIGN_VERSION "([^"]+)"',
                    open(os.path.join(ROOT, 'modules/gui/qt/util/redesign_version.hpp'), encoding='utf-8').read()).group(1)
tag = 'v' + version
if git('status', '--porcelain', '--untracked-files=no'):
    sys.exit('Commit everything first')
head = git('rev-parse', 'HEAD')
pr = json.loads(gh('pr', 'view', str(args.pr), '--json', 'headRefOid,state,mergeCommit'))
if pr['state'] == 'OPEN' and pr['headRefOid'] != head:
    sys.exit('The pull request is at %s, this folder at %s' % (pr['headRefOid'][:10], head[:10]))
checks = gh('pr', 'checks', str(args.pr), check=False).splitlines()
bad = [c.split('\t')[0] for c in checks if len(c.split('\t')) > 1 and c.split('\t')[1] not in ('pass', 'skipping')]
if bad or not checks:
    sys.exit('Checks not passed: %s' % (', '.join(bad) or 'none found'))
tree = git('rev-parse', 'HEAD^{tree}')
# the commit GitHub built: the pull request's merge with main (the same after it is merged)
runs = json.loads(gh('run', 'list', '--commit', head, '--limit', '20', '--json', 'databaseId,name,conclusion,event'))
runs = {r['name']: r['databaseId'] for r in runs if r['conclusion'] == 'success' and r['event'] == 'pull_request'}
for name in ('Windows build', 'Linux build', 'macOS build'):
    if name not in runs:
        sys.exit('No successful "%s" of this commit' % name)
artifacts = {}
for name in ('Windows build', 'Linux build', 'macOS build'):
    for a in api('repos/%s/actions/runs/%d/artifacts?per_page=100' % (REPO, runs[name]))['artifacts']:
        artifacts[a['name']] = runs[name]
built = {m.group(1) for n in artifacts for m in [re.match(r'vlc-redesign-win64-([0-9a-f]{40})$', n)] if m}
if len(built) != 1:
    sys.exit('The Windows package of this commit is not among the artifacts')
built = built.pop()
built_tree = api('repos/%s/git/commits/%s' % (REPO, built))['tree']['sha']
if built_tree != tree:
    sys.exit('GitHub built %s (tree %s), not this commit\'s tree %s: merge main into the branch, wait for its checks'
             % (built[:10], built_tree[:10], tree[:10]))
step('%s: pull request #%d at %s, %d checks passed, built as %s (the same tree)'
     % (version, args.pr, head[:10], len(checks), built[:10]))

# --- 2. its builds ----------------------------------------------------------------------------------------------
work = os.path.join(args.work, version)
os.makedirs(work, exist_ok=True)
wanted = {'win64': 'vlc-redesign-win64-', 'winarm64': 'vlc-redesign-winarm64-',
          'linux-x86_64': 'vlc-redesign-linux-x86_64-', 'linux-aarch64': 'vlc-redesign-linux-aarch64-',
          'macos-arm64': 'vlc-redesign-macos-arm64-', 'macos-x86_64': 'vlc-redesign-macos-x86_64-'}
for folder, prefix in wanted.items():
    name = prefix + built
    if name not in artifacts:
        sys.exit('No artifact %s' % name)
    out = os.path.join(work, folder)
    marker = os.path.join(out, '.downloaded-' + built[:10])
    if not os.path.exists(marker):
        shutil.rmtree(out, ignore_errors=True)
        gh('run', 'download', str(artifacts[name]), '-n', name, '-D', out)
        open(marker, 'w').close()
for arch in ('x86_64', 'aarch64'):
    folder = os.path.join(work, 'linux-' + arch)
    for line in open(os.path.join(folder, 'SHA256SUMS-linux-%s.txt' % arch), encoding='utf-8'):
        digest, name = line.split()
        if sha256(os.path.join(folder, name.lstrip('*'))) != digest:
            sys.exit('%s: not the checksum built with it' % name)
for arch in ('arm64', 'x86_64'):
    dmg = os.path.join(work, 'macos-' + arch, 'vlc-redesign-%s-macos-%s.dmg' % (version, arch))
    if sha256(dmg) != open(dmg + '.sha256', encoding='utf-8').read().split()[0]:
        sys.exit('%s: not the checksum built with it' % os.path.basename(dmg))
step('the builds of %s in %s (Linux and macOS checksums good)' % (built[:10], work))

# --- 3. the Windows files -----------------------------------------------------------------------------------------
dist = args.dist
packages = {'win64': os.path.join(HOME, 'source', 'vlc-redesign-release'),
            'winarm64': os.path.join(HOME, 'source', 'vlc-redesign-release-arm64')}
for platform, arch in (('win64', 'x86_64'), ('winarm64', 'aarch64')):
    if os.path.exists(os.path.join(dist, 'vlc-redesign-%s-%s-setup.exe' % (version, platform))) and \
            os.path.exists(os.path.join(work, platform, '.packaged-' + built[:10])):
        continue
    # (robocopy: 0-7 are successes; the folder's own portable\ stays)
    r = subprocess.run(['robocopy', os.path.join(work, platform), packages[platform], '/MIR', '/XD', 'portable',
                        '/XF', '.downloaded-*', '.packaged-*', '/NFL', '/NDL', '/NJH', '/NJS', '/NP'],
                       capture_output=True)
    if r.returncode > 7:
        sys.exit('robocopy to %s failed (%d)' % (packages[platform], r.returncode))
    # (-NoSumsSignature: the list of every file is signed once, below)
    mk = (['-SkipBuild', '-NoSumsSignature', '-Arch', arch, '-Package', packages[platform], '-Dist', dist]
          + ([] if args.full else ['-Update']))
    powershell(os.path.join(ROOT, 'extras', 'juxtopposed', 'make-release.ps1'), *mk)
    r = run([sys.executable, os.path.join(ROOT, 'extras', 'juxtopposed', 'ci', 'pe-check.py'), '--arch',
             'x64' if platform == 'win64' else 'arm64', packages[platform]], quiet=True)
    if 'failures: 0' not in r.stdout:
        sys.exit('pe-check of %s:\n%s' % (platform, r.stdout[-1500:]))
    open(os.path.join(work, platform, '.packaged-' + built[:10]), 'w').close()
    step('%s: installer, zip and small update made, the package checked (pe-check: 0 failures)' % platform)

# --- 4. every file, its checksum, the signature ------------------------------------------------------------------
for arch in ('x86_64', 'aarch64'):
    for name in ('vlc-redesign-%s-%s.AppImage' % (version, arch), 'vlc-redesign-%s-%s.AppImage.zsync' % (version, arch),
                 'vlc-redesign-%s-linux-%s.tar.xz' % (version, arch)):
        shutil.copy2(os.path.join(work, 'linux-' + arch, name), os.path.join(dist, name))
    if args.full:
        for name in os.listdir(os.path.join(work, 'linux-' + arch)):
            if name.endswith(('.deb', '.rpm', '.flatpak')):
                shutil.copy2(os.path.join(work, 'linux-' + arch, name), os.path.join(dist, name))
for arch in ('arm64', 'x86_64'):
    name = 'vlc-redesign-%s-macos-%s.dmg' % (version, arch)
    shutil.copy2(os.path.join(work, 'macos-' + arch, name), os.path.join(dist, name))
if args.apk:
    shutil.copy2(args.apk, os.path.join(dist, 'vlc-redesign-%s-android.apk' % version))
tv = 'Juxto-Media-Player-TV-%s-source.zip' % version
if args.tv_zip:
    shutil.copy2(args.tv_zip, os.path.join(dist, tv))
names = sorted(n for n in os.listdir(dist)
               if (n.startswith('vlc-redesign-%s-' % version) or n.startswith('vlc-redesign_%s_' % version)
                   or n.startswith('vlc-redesign-livetv') and version in n or n == tv)
               and not n.endswith(('.md', '.sha256')))
deltas = [n for n in names if n.endswith('.vrdelta')]
if len(deltas) != 2:
    sys.exit('A release has one small update for each Windows architecture, not %d: %s' % (len(deltas), deltas))
sums = os.path.join(dist, 'SHA256SUMS.txt')
with open(sums, 'w', encoding='ascii', newline='\n') as f:
    f.write(''.join('%s  %s\n' % (sha256(os.path.join(dist, n)), n) for n in names))
key = os.path.join(ROOT, 'extras', 'juxtopposed', 'signing', 'release-key.ps1')
if os.path.exists(sums + '.sig'):
    os.remove(sums + '.sig')
powershell(key, 'sign', sums)
powershell(key, 'verify', sums)
step('%d files in SHA256SUMS.txt, signed and verified%s%s'
     % (len(names), '' if args.apk else ' (no APK: --apk)', ' (with the Samsung TV app)' if tv in names else ''))

# --- 5. the notes --------------------------------------------------------------------------------------------------
run([sys.executable, os.path.join(HERE, 'release-notes.py'), version, dist])
notes = os.path.join(dist, 'release-notes-%s.md' % version)
step('notes: %s' % notes)
files = [os.path.join(dist, n) for n in names] + [sums, sums + '.sig']
if not args.publish:
    print('\nReady, not published: look at the notes and the files, then run again with --publish.')
    sys.exit(0)

# --- 6. the tag, the merge -----------------------------------------------------------------------------------------
tagged = run(['git', '-C', ROOT, 'rev-parse', '-q', '--verify', 'refs/tags/%s^{commit}' % tag], check=False)
if tagged and tagged != head:
    sys.exit('%s is %s, not this commit' % (tag, tagged[:10]))
if not tagged:
    git('tag', '-a', tag, '-m', 'VLC Media Player Redesign %s' % version, head)
if run(['gh', 'api', 'repos/%s/git/refs/tags/%s' % (REPO, tag)], check=False, quiet=True).returncode != 0:
    r = subprocess.run(['git', '-C', ROOT, 'push', 'github', tag])
    if r.returncode != 0:
        sys.exit('The tag was not pushed (the hook above says why)')
if pr['state'] == 'OPEN':
    gh('pr', 'merge', str(args.pr), '--merge')
merged = json.loads(gh('pr', 'view', str(args.pr), '--json', 'mergeCommit'))['mergeCommit']['oid']
if api('repos/%s/git/commits/%s' % (REPO, merged))['tree']['sha'] != tree:
    sys.exit('main (%s) has not the tree of the tag' % merged[:10])
step('%s on %s, pushed; pull request #%d merged as %s (the same tree)' % (tag, head[:10], args.pr, merged[:10]))

# --- 7. the draft, its digests ---------------------------------------------------------------------------------------
if run(['gh', 'release', 'view', tag, '--repo', REPO], check=False, quiet=True).returncode != 0:
    gh('release', 'create', tag, '--draft', '--verify-tag', '--title', 'VLC Media Player Redesign %s' % version,
       '--notes-file', notes, *files)
assets = {a['name']: a for a in json.loads(gh('release', 'view', tag, '--json', 'assets'))['assets']}
wrong = [n for n in names if n not in assets or assets[n].get('digest') != 'sha256:' + sha256(os.path.join(dist, n))]
wrong += [n for n in assets if n not in names + ['SHA256SUMS.txt', 'SHA256SUMS.txt.sig']]
for n in ('SHA256SUMS.txt', 'SHA256SUMS.txt.sig'):
    if n not in assets or assets[n]['size'] != os.path.getsize(os.path.join(dist, n)):
        wrong.append(n)
if wrong:
    sys.exit('The draft is not the signed list: %s' % ', '.join(wrong))
step('the draft has the %d files of the signed list, each with its digest' % len(names))

# --- 8. published ----------------------------------------------------------------------------------------------------
gh('release', 'edit', tag, '--draft=false', '--latest')
check = os.path.join(work, 'published')
shutil.rmtree(check, ignore_errors=True)
os.makedirs(check)
gh('release', 'download', tag, '-p', 'SHA256SUMS.txt', '-p', 'SHA256SUMS.txt.sig', '-D', check)
if open(os.path.join(check, 'SHA256SUMS.txt'), 'rb').read() != open(sums, 'rb').read():
    sys.exit('The published SHA256SUMS.txt is not the local one')
powershell(key, 'verify', os.path.join(check, 'SHA256SUMS.txt'))
step('published as the latest: https://github.com/%s/releases/tag/%s (its signature verified)' % (REPO, tag))
print('\nNext: the in-app update on a copy of the release before; the post-release commit (the next version).')
