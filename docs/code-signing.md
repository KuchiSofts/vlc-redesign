# Code signing (Windows)

VLC Redesign's Windows files are to be signed through [SignPath Foundation](https://signpath.org/)'s free program
for open-source projects (decision 2 of the [master plan](MASTER-PLAN.md#17-decisions-for-the-project-owner):
*SignPath free OSS signing*). The certificate is issued to SignPath Foundation, not to the project, and SignPath
signs only what this repository's GitHub workflow built, after an approval by hand.

**Status:** prepared, not active. The workflow skips signing until the SignPath setup below exists, and nothing has
been applied for, submitted or published yet. The policy that users read is
[CODE_SIGNING_POLICY.md](../CODE_SIGNING_POLICY.md).

| File                                                        | What it is                                                           |
|-------------------------------------------------------------|----------------------------------------------------------------------|
| [`build-windows.yml`](../.github/workflows/build-windows.yml) | Jobs `sign` and `release`, run only when started by hand with a signing policy |
| [`signing/artifact-configuration.xml`](../extras/juxtopposed/signing/artifact-configuration.xml) | SignPath artifact configuration `package`: the .exe and .dll files of the package |
| [`signing/installer-configuration.xml`](../extras/juxtopposed/signing/installer-configuration.xml) | SignPath artifact configuration `installer`: the installer          |
| [`make-release.ps1`](../extras/juxtopposed/make-release.ps1) `-Signed` | Packages a folder whose binaries are signed already, without changing them |
| [`CODE_SIGNING_POLICY.md`](../CODE_SIGNING_POLICY.md)        | The policy SignPath Foundation requires: what is signed, team roles, privacy |

## What is signed, and in which order

| File                                              | Signed | How                                                   |
|---------------------------------------------------|--------|-------------------------------------------------------|
| Every `.exe` and `.dll` of the package (416 in 0.2.6: 5 programs, `libvlc.dll`, `libvlccore.dll`, 409 plugins) | Yes | Signing request 1, artifact configuration `package` |
| `vlc-redesign-<version>-win64-setup.exe`          | Yes    | Signing request 2, artifact configuration `installer` |
| `vlc-redesign-<version>-win64.zip`                | No (a zip has no signature) | Holds the signed binaries        |
| `*.vrdelta` small updates, `SHA256SUMS.txt`       | No     | Data, made from the signed files and checked by SHA-256 |
| `unins000.exe`, written by the installer          | Not yet | See [Later](#later)                                  |

Signing changes the bytes of every binary (the signature is added to the file, and the PE checksum changes), so
everything that reads or copies those bytes comes after it, and nothing may change a binary once it is signed:

1. **`win64`** (unchanged): the package is built in Docker, as for every pull request.
2. **`launch-test`** (unchanged): the package starts and plays on Windows.
3. **`sign`**: the PE time stamps are set to 0 (`make-delta.py normalize`, which `make-release.ps1` does to an
   unsigned package: the time stamp is part of what a signature covers), the package goes to SignPath (request 1), and
   the signed package comes back. The job checks that every file came back, that every binary changed and that no
   other file did, then uploads it as `vlc-redesign-win64-<policy>-<commit>`.
4. **`release`** (Windows): `make-release.ps1 -Signed` on the signed package. It checks that every binary carries a
   signature and leaves them as they are, then makes the plugin cache (`plugins.dat` records the signed sizes), the
   portable zip, the installer (which holds the signed files) and the small updates from the latest releases (each
   checked by applying it to the old release and comparing every file with the signed package). The installer goes
   to SignPath (request 2); `SHA256SUMS.txt` is then made again, since the installer's bytes changed. The release
   files are uploaded as `vlc-redesign-<version>-<policy>-<commit>`.
5. **By hand:** you check the files and publish them.

The release is made by the workflow, not on this PC: SignPath Foundation signs only what a GitHub-hosted runner built
from this repository (it checks the repository, the branch, the commit and the workflow run of each request), so a
package or an installer made locally can't be signed.

## What changes for users

- **Publisher:** Windows shows **SignPath Foundation** as the verified publisher (in SmartScreen, in the
  administrator prompt of *Install for everyone*, in a file's Properties → Digital Signatures), not KuchiSofts.
  *Apps* in the Windows settings keeps showing KuchiSofts, from the installer.
- **SmartScreen:** the *Unknown publisher* warning goes away. Whether SmartScreen still shows *Windows protected
  your PC* depends on the reputation Microsoft gives the certificate and the file through their downloads; a new
  release can still get it at first, with SignPath Foundation as the publisher. Microsoft gives no timeline.
- **Smart App Control** (Windows 11) blocks unsigned programs that Microsoft's cloud service doesn't know; signed
  ones run.
- **Checking a download:** Properties → Digital Signatures shows *SignPath Foundation* and the signing time, or
  `Get-AuthenticodeSignature <file>` says `Valid`. `SHA256SUMS.txt` stays.
- **Updates** work as before: the updater checks every download against `SHA256SUMS.txt`, made from the signed
  files, and a small update rebuilds the signed files byte for byte. The first update from an unsigned release
  replaces every binary. Small updates get bigger, see [Later](#later).
- **Builds from source** are unchanged and unsigned: only this repository's GitHub build can be signed.

## Before applying: what the project still has to change

SignPath Foundation's conditions are at <https://signpath.org/terms>. The first is met; the others are not yet:

1. **The binaries' version resources (done).** SignPath Foundation requires the artifact configuration to check the
   product name (the project's name) and the product version (the same in all files of a build) of every signed
   binary. Every `.exe` and `.dll` of the package says `ProductName` *VLC Redesign*, `CompanyName` *KuchiSofts*, and
   `VLC_REDESIGN_VERSION` as `ProductVersion` and `FileVersion` (for instance "0.3.0", with `PRODUCTVERSION
   0,3,0,0`), with a `FileDescription` of its own; `LegalCopyright` keeps VideoLAN and the VLC authors and adds
   VLC Redesign's, `LegalTrademarks` stays, and `Comments` names the VLC version it is based on.
   - The version is written only in `modules/gui/qt/util/redesign_version.hpp`: `configure.ac` (and `meson.build`)
     read it, and configure runs again when it changes. Its values fill `bin/vlc_win32_rc.rc.in` (`vlc.exe`, and
     `vlc-cache-gen.exe` and `vlc-preparser.exe` with their own names), `bin/vlc_waveform_rc.rc.in`,
     `lib/libvlc_win32_rc.rc.in`, `src/libvlccore_win32_rc.rc.in`, `modules/module.rc.in` (every plugin) and
     `extras/juxtopposed/updater/vlc-redesign-update.rc.in` (`build_updater` in
     `extras/juxtopposed/win64-docker.sh`).
   - Check a package (it must list nothing):
     ```powershell
     $v = '0.3.0'   # VLC_REDESIGN_VERSION
     Get-ChildItem <package> -Recurse -File -Include *.exe, *.dll |
         Where-Object { $_.VersionInfo.ProductName -ne 'VLC Redesign' -or $_.VersionInfo.ProductVersion -ne $v } |
         Select-Object Name, @{ n = 'Product'; e = { $_.VersionInfo.ProductName } }, @{ n = 'Version'; e = { $_.VersionInfo.ProductVersion } }
     ```
   The installer already says *VLC Redesign* and its version (Inno Setup takes them from the `.iss`).
2. **Privacy.** SignPath Foundation requires that software sending user data to services the user didn't name
   describes it in a privacy policy, shows that policy during the installation, and offers installation options to
   turn those features off. Two features are on out of the box: the update check (GitHub) and the automatic online
   subtitles (the video's title, file name and hash to IMDb, Wikidata, Cinemeta, OpenSubtitles.org, Wizdom, Gestdown,
   YIFY Subtitles). The policy describes them; the installer neither shows it nor has the options. Either:
   - the installer shows the privacy policy (a link on its Welcome page) and its Options page gets two toggles,
     *Check for updates automatically* and *Find subtitles online*, that set the two settings; the portable zip has
     no installation, so the first-run setup asks the same; or
   - both are off until the user turns them on (the first-run setup asks, unticked), and the policy can then use
     SignPath's sentence: *This program will not transfer any information to other networked systems unless
     specifically requested by the user or the person installing or operating it*, with the list of what the user
     can turn on.
3. **"Code signing policy" on the home page and the download page.** SignPath Foundation requires the words *Code
   signing policy* there, as a heading or a link. Add to `README.md`, for instance before *License*:
   ```markdown
   ## Code signing policy

   Free code signing provided by [SignPath.io](https://about.signpath.io/), certificate by
   [SignPath Foundation](https://signpath.org/). What is signed, the team roles and the privacy policy:
   [CODE_SIGNING_POLICY.md](CODE_SIGNING_POLICY.md).
   ```
   and a line to the *Download* section and to the notes of every signed release:
   ```markdown
   Windows files signed: [Code signing policy](https://github.com/KuchiSofts/vlc-redesign/blob/main/CODE_SIGNING_POLICY.md)
   ```
   After the first signed release, the SmartScreen note under *Updates* in the README changes too.
4. **Multi-factor authentication** for everyone in the team, on GitHub (<https://github.com/settings/security>) and
   on SignPath.

Points to raise in the application, since SignPath Foundation decides on them:

- **A fork whose upstream branch is not signed.** Signing a modified upstream project is allowed when the upstream
  publishes signed builds and the fork's release branches are based on upstream branches that are usually signed.
  VideoLAN signs its VLC 3 releases, but this fork is based on VLC's master (4.0 development), and VideoLAN's
  nightly builds of it are not signed (checked on the build of 2026-10-23). The repository is also not a fork in
  GitHub's sense (VLC lives on VideoLAN's GitLab); the README says it is a fork.
- **Reputation.** SignPath Foundation wants a verifiable reputation for programs that people download and run. The
  repository is from 2026-10-20; they may answer that it is too new.
- **The name.** *VLC* is VideoLAN's trademark; the installer and the README already say that the build is
  unofficial.
- **Third-party libraries.** VLC's contribs (FFmpeg, Qt, ...) come from VideoLAN's prebuilt archive of the recipes
  in `contrib/`, and the build runs in VideoLAN's Docker image; both are downloaded by the workflow.

## Checklist

### 1. Apply

1. Read the conditions: <https://signpath.org/terms>.
2. Apply with the form at <https://signpath.org/apply>, with an e-mail address you read (the rest of it happens by
   e-mail). What to give:
   - **Project:** VLC Redesign
   - **Repository:** <https://github.com/KuchiSofts/vlc-redesign> (public; GPL-2.0-or-later, [`COPYING`](../COPYING))
   - **Download page:** <https://github.com/KuchiSofts/vlc-redesign/releases> (free)
   - **Code signing policy:** <https://github.com/KuchiSofts/vlc-redesign/blob/main/CODE_SIGNING_POLICY.md>
   - **What to sign:** the Windows x64 package (the .exe and .dll files) and its installer (Inno Setup); the portable
     zip holds the same signed files.
   - **Build:** GitHub Actions on GitHub-hosted runners,
     <https://github.com/KuchiSofts/vlc-redesign/blob/main/.github/workflows/build-windows.yml>
   - **Team:** KuchiSofts (author, reviewer, approver)
   - **Description** (to paste):
     > VLC Redesign is a free, open-source build of VLC 4 for Windows with a new interface, designed by Juxtopposed
     > (CC BY 4.0) and implemented in Qt/QML. It is an unofficial fork of VLC (GPL-2.0-or-later), not made or
     > endorsed by VideoLAN; VLC's changes are merged through reviewed pull requests. Releases are free on GitHub:
     > an installer and a portable zip for Windows 10 and 11, x64. GitHub Actions cross-compiles the package on
     > GitHub-hosted runners in VideoLAN's official build image; we would sign its .exe and .dll files and the
     > installer, each release approved by hand.
   - The points of the previous section: the fork, the libraries, the name.
3. Wait for the answer; SignPath may ask questions.

### 2. Set up SignPath (once accepted)

Follow SignPath's onboarding e-mail; sign in at <https://app.signpath.io> with an account that has multi-factor
authentication. In the end the SignPath project needs:

- **Repository URL:** `https://github.com/KuchiSofts/vlc-redesign`.
- **Trusted build system:** the predefined *GitHub.com*, added to the organization and linked to the project
  ([SignPath's guide](https://docs.signpath.io/trusted-build-systems/github)). The SignPath GitHub App is optional.
- **Two artifact configurations**, with these slugs (the workflow uses them):
  - `package`: paste [`extras/juxtopposed/signing/artifact-configuration.xml`](../extras/juxtopposed/signing/artifact-configuration.xml)
  - `installer`: paste [`extras/juxtopposed/signing/installer-configuration.xml`](../extras/juxtopposed/signing/installer-configuration.xml)
- **Two signing policies**, with the slugs `test-signing` and `release-signing` (the choices of the workflow; if
  SignPath names them otherwise, change the `sign` options in `build-windows.yml`). For `release-signing`: approval
  by hand, approver KuchiSofts; origin verification on; allowed branch `main`.
- **A CI user** that is a submitter of both policies (with origin verification, people can't be submitters), and its
  API token. The token is shown once.
- The **organization ID** (a GUID: in the address of the pages of app.signpath.io, and in the organization's
  settings) and the **project slug**.

### 3. GitHub secret and variables

In the repository's Settings → Secrets and variables → Actions:

| Kind     | Name                        | Value                    | Page |
|----------|-----------------------------|--------------------------|------|
| Secret   | `SIGNPATH_API_TOKEN`        | The CI user's API token  | <https://github.com/KuchiSofts/vlc-redesign/settings/secrets/actions> |
| Variable | `SIGNPATH_ORGANIZATION_ID`  | The organization ID      | <https://github.com/KuchiSofts/vlc-redesign/settings/variables/actions> |
| Variable | `SIGNPATH_PROJECT_SLUG`     | The project slug         | same page |

Or with `gh` (it asks for the token):

```powershell
gh secret set SIGNPATH_API_TOKEN --repo KuchiSofts/vlc-redesign
gh variable set SIGNPATH_ORGANIZATION_ID --repo KuchiSofts/vlc-redesign --body '<organization id>'
gh variable set SIGNPATH_PROJECT_SLUG --repo KuchiSofts/vlc-redesign --body '<project slug>'
```

The policy is chosen each time the workflow is started. Until `SIGNPATH_ORGANIZATION_ID` exists, the jobs `sign` and
`release` are skipped in every run.

### 4. Check the setup with test-signing

1. Push to `main` with the workflow change (the *Sign* choice appears once `build-windows.yml` is there).
2. Actions → [Windows build](https://github.com/KuchiSofts/vlc-redesign/actions/workflows/build-windows.yml) →
   *Run workflow*: branch `main`, *Sign* `test-signing`.
3. The four jobs run; `release` warns that the signatures are not trusted by Windows (the test certificate is
   self-signed): that is expected. A refused request says why in SignPath (for instance a product name or version
   that doesn't match: prerequisite 1).
4. Never publish anything from a test-signing run.

### 5. A signed release

1. As usual: the version in `modules/gui/qt/util/redesign_version.hpp`, `CHANGELOG.md`, the commit on `redesign`,
   pushed to `main` when the release is uploaded.
2. Actions → [Windows build](https://github.com/KuchiSofts/vlc-redesign/actions/workflows/build-windows.yml) →
   *Run workflow*: branch `main`, *Sign* `release-signing`.
3. After the build and the launch test, SignPath e-mails a signing request for the package: in SignPath, check
   that it comes from `main` and the commit to release, then approve it. Each request waits up to 3 hours; after
   that the job fails and a new run is needed (start a new run rather than re-running jobs: SignPath may refuse
   re-runs).
4. The `release` job makes the release files and sends the installer: approve the second request.
5. Download the artifact `vlc-redesign-<version>-release-signing-<commit>` from the run's page and unzip it: the zip,
   the installer, the small updates and `SHA256SUMS.txt`.
6. Check them on this PC:
   ```powershell
   $d = '<the unzipped folder>'; $v = '<version>'
   Get-AuthenticodeSignature "$d\vlc-redesign-$v-win64-setup.exe" | Format-List Status, SignerCertificate
   Expand-Archive "$d\vlc-redesign-$v-win64.zip" "$env:TEMP\vlc-redesign-check" -Force
   Get-ChildItem "$env:TEMP\vlc-redesign-check" -Recurse -File -Include *.exe, *.dll |
       Where-Object { (Get-AuthenticodeSignature -LiteralPath $_.FullName).Status -ne 'Valid' }   # lists nothing
   Get-Content "$d\SHA256SUMS.txt"; Get-FileHash "$d\*" -Algorithm SHA256 | Format-Table Hash, Path
   ```
   `Status` is `Valid` and the signer is SignPath Foundation. The clean-PC test in Windows Sandbox works as before.
7. Copy the files into the release folder (so the next local build finds this release's zip) and publish them as
   usual, with the *Code signing policy* line in the notes.

If the `release` job fails for a reason that has nothing to do with signing, the signed package
(`vlc-redesign-win64-release-signing-<commit>`) can be packaged here: unzip it and run
`.\extras\juxtopposed\make-release.ps1 -Signed -Package <folder>`. The installer made so is not signed (SignPath
signs only what the workflow built), so prefer fixing the job and starting a new run.

## Later

- **The uninstaller.** Inno Setup writes `unins000.exe` at installation, unsigned; Smart App Control may block it.
  Inno Setup can embed a signed uninstaller (`SignedUninstaller=yes` with a `SignedUninstallerDir` that holds the
  uninstaller signed once per Inno Setup version and setup settings): a third signing request.
- **Smaller small updates.** Every release signs every binary again, so a binary whose code didn't change still
  differs from the last release by its signature, and `make-delta.py` puts it in the update: about 5 to 7 MB per
  small update instead of 0.3 to 1.4 MB (estimated from the 0.2.6 package, whose 254 binaries under 64 KB would go
  whole). Fix: let
  `comparable()` in `make-delta.py` leave out the signature (the certificate table, its entry in the PE header and
  the checksum), so that an unchanged binary keeps the copy in place, signed by an earlier release.
- **SignPath may require more** later (SBOM files, only signed files inside signed packages): see its conditions.
- macOS (decision 3) and the other platforms are not covered by this.
