#!/bin/bash
# Copyright (C) 2026 VLC authors and VideoLAN
# SPDX-License-Identifier: GPL-2.0-or-later
# Installs one Linux package of VLC Redesign in a clean container of a
# distribution, checks it, runs a UI tour under Xvfb (and a start on headless
# Weston), removes it and lists what was left behind. Run as root in the
# container, with the packages in /pkg, the test clip in /media (clip.mp4 and
# its Hebrew subtitle clip.he.srt), this folder in /harness, results in /out:
#   docker run --rm -v "$PWD/build-qt68/packages:/pkg:ro" -v "$PWD/build-qt68/pkgtest/media:/media:ro" \
#       -v "$PWD/extras/juxtopposed:/harness:ro" -v "$PWD/build-qt68/pkgtest/out:/out" \
#       ubuntu:24.04 bash /harness/linux-pkg-test.sh deb ubuntu-24.04
# KIND: deb, rpm, appimage, tarball or flatpak (the Flatpak needs --privileged
# and the KDE 6.8 runtime, e.g. -v vlc-redesign-flatpak:/var/lib/flatpak).
# The tour is linux-pkg-tour.txt (clicks at 1280x800, keys, captures). Audio
# is silenced with --aout=dummy. Exit status 0 when every check passed;
# /out/NAME/summary.txt lists them.
set -uo pipefail
kind=$1
name=${2:-$kind}
out=/out/$name
rm -rf "$out"
mkdir -p "$out"
# (written by the test user too)
chmod 777 "$out"
summary="$out/summary.txt"
: > "$summary"
failed=0
pass() { echo "PASS $*" | tee -a "$summary"; }
fail() { echo "FAIL $*" | tee -a "$summary"; failed=1; }
info() { echo "INFO $*" | tee -a "$summary"; }
. /etc/os-release
info "$PRETTY_NAME, $(getconf GNU_LIBC_VERSION), package: $kind"

# --- the tools of the test (not counted in what the package leaves) --------
if command -v apt-get > /dev/null; then pm=apt
elif command -v dnf > /dev/null; then pm=dnf
elif command -v pacman > /dev/null; then pm=pacman
else fail "no package manager known"; exit 1
fi
case $pm in
    apt) export DEBIAN_FRONTEND=noninteractive
         apt-get update -qq
         apt-get install -y -qq --no-install-recommends xvfb xdotool imagemagick dbus dbus-x11 \
             fonts-dejavu-core weston file binutils xz-utils desktop-file-utils locales \
             ${FLATPAK:+flatpak} > "$out/tools.log" 2>&1
         # (not ca-certificates: the package must bring it)
         sed -i -E 's/^# *(en_US|he_IL)(\.UTF-8 UTF-8)/\1\2/' /etc/locale.gen
         locale-gen en_US.UTF-8 he_IL.UTF-8 >> "$out/tools.log" 2>&1 ;;
    dnf) dnf install -y -q xorg-x11-server-Xvfb xdotool ImageMagick dbus-daemon dbus-x11 \
             dejavu-sans-fonts weston file binutils xz desktop-file-utils findutils procps-ng \
             glibc-langpack-en glibc-langpack-he ${FLATPAK:+flatpak} > "$out/tools.log" 2>&1 ;;
    pacman) pacman -Sy --noconfirm --needed xorg-server-xvfb xdotool imagemagick dbus ttf-dejavu \
             weston file binutils xz desktop-file-utils ${FLATPAK:+flatpak} > "$out/tools.log" 2>&1
         sed -i -E 's/^# *(en_US|he_IL)(\.UTF-8 UTF-8)/\1\2/' /etc/locale.gen
         locale-gen en_US.UTF-8 he_IL.UTF-8 >> "$out/tools.log" 2>&1 ;;
esac
[ $? -eq 0 ] || info "some test tools did not install (tools.log)"
command -v Xvfb > /dev/null || { fail "Xvfb is missing"; exit 1; }
id tester > /dev/null 2>&1 || useradd -m -u 1500 tester
snapshot() {
    find / -xdev \( -path /proc -o -path /sys -o -path /dev -o -path /tmp -o -path /run -o -path /out \
        -o -path /pkg -o -path /media -o -path /harness -o -path /var/cache -o -path /var/log \
        -o -path /var/lib/apt -o -path /var/lib/dpkg -o -path /var/lib/dnf -o -path /var/lib/rpm \
        -o -path /usr/lib/sysimage/rpm -o -path /var/lib/pacman -o -path /home/tester \
        -o -path /var/lib/flatpak/repo -o -path /var/tmp \) -prune -o -print 2>/dev/null | sort
}
snapshot > /tmp/before.txt

# --- install ----------------------------------------------------------------
main=$(ls /pkg/vlc-redesign_*.deb /pkg/vlc-redesign-[0-9]*.rpm 2>/dev/null | grep -E "\.$kind\$" | head -n 1)
tv=$(ls /pkg/vlc-redesign-livetv*."$kind" 2>/dev/null | head -n 1)
vlc=
case $kind in
    deb) apt-get install -y -q "$main" "$tv" > "$out/install.log" 2>&1 \
            && pass "installed with apt: $(basename "$main") $(basename "$tv")" \
            || { fail "apt install (install.log)"; tail -n 20 "$out/install.log"; exit 1; }
         vlc=/usr/bin/vlc-redesign ;;
    rpm) dnf install -y "$main" "$tv" > "$out/install.log" 2>&1 \
            && pass "installed with dnf: $(basename "$main") $(basename "$tv")" \
            || { fail "dnf install (install.log)"; tail -n 20 "$out/install.log"; exit 1; }
         rpm -V vlc-redesign vlc-redesign-livetv > "$out/rpm-verify.log" 2>&1 \
            && pass "rpm -V" || fail "rpm -V (rpm-verify.log)"
         vlc=/usr/bin/vlc-redesign ;;
    appimage) app=$(ls /pkg/vlc-redesign-*-x86_64.AppImage | head -n 1)
         install -m 755 "$app" /opt/test.AppImage
         # (no FUSE in a container)
         printf '#!/bin/sh\nexec /opt/test.AppImage --appimage-extract-and-run "$@"\n' > /usr/local/bin/vlc-test
         chmod 755 /usr/local/bin/vlc-test
         vlc=/usr/local/bin/vlc-test
         pass "AppImage copied: $(basename "$app")" ;;
    tarball) tar -C /opt -xJf "$(ls /pkg/vlc-redesign-*-linux-x86_64.tar.xz | head -n 1)"
         vlc=/opt/vlc-redesign/vlc
         pass "tarball unpacked" ;;
    flatpak) fp=$(ls /pkg/vlc-redesign-*.flatpak | head -n 1)
         flatpak remote-add --system --if-not-exists flathub https://dl.flathub.org/repo/flathub.flatpakrepo
         flatpak install --system -y --noninteractive "$fp" > "$out/install.log" 2>&1 \
            && pass "flatpak installed: $(basename "$fp")" \
            || { fail "flatpak install (install.log)"; tail -n 20 "$out/install.log"; exit 1; }
         printf '#!/bin/sh\nexec flatpak run org.kuchisofts.VLCRedesign "$@"\n' > /usr/local/bin/vlc-test
         chmod 755 /usr/local/bin/vlc-test
         vlc=/usr/local/bin/vlc-test ;;
esac

# --- files ------------------------------------------------------------------
root=/opt/vlc-redesign
[ "$kind" = appimage ] && { (cd /tmp && /opt/test.AppImage --appimage-extract > /dev/null); root=/tmp/squashfs-root; }
[ "$kind" = flatpak ] && root=$(flatpak info --show-location org.kuchisofts.VLCRedesign)/files/vlc-redesign
if [ -x "$root/bin/vlc" ]; then
    # libraries not found (with the launcher's paths)
    export LDP="$root/lib:$root/lib/vlc:$root/qt/lib:$root/syslib"
    if [ "$kind" = flatpak ]; then
        info "ldd skipped in the Flatpak (its libraries are those of the runtime)"
    else
        find "$root" -type f \( -name '*.so*' -o -perm -u+x \) -print0 \
            | xargs -0 file | grep ELF | cut -d: -f1 | while read -r f; do
                LD_LIBRARY_PATH="$LDP" ldd "$f" 2>/dev/null | grep 'not found' | sed "s|^|$f: |"
            done > "$out/ldd-missing.txt"
        # (Qt's plugins for what VLC doesn't use may name absent libraries)
        grep -v -E '/qt/(plugins|qml)/' "$out/ldd-missing.txt" > "$out/ldd-missing-vlc.txt"
        if [ -s "$out/ldd-missing-vlc.txt" ]; then
            fail "libraries not found: $(cut -d: -f2- "$out/ldd-missing-vlc.txt" | sort -u | tr -s ' \t\n' ' ')"
        else
            pass "ldd: every library of VLC found ($(wc -l < "$out/ldd-missing.txt") lines in Qt's unused plugins)"
        fi
    fi
    n=$(ls "$root"/share/locale/*/LC_MESSAGES/vlc.mo 2>/dev/null | wc -l)
    [ "$n" -ge 72 ] && pass "translations: $n languages" || fail "translations: $n languages (72 expected)"
    [ -f "$root/lib/vlc/plugins/misc/libiptv_plugin.so" ] && pass "Live TV plugin installed" \
        || fail "Live TV plugin missing"
else
    fail "no $root/bin/vlc"
fi
case $kind in
    deb|rpm)
        d=/usr/share/applications/org.kuchisofts.VLCRedesign.desktop
        desktop-file-validate "$d" > "$out/desktop.log" 2>&1 && pass "desktop entry valid" \
            || fail "desktop entry (desktop.log)"
        for s in scalable/apps/org.kuchisofts.VLCRedesign.svg 256x256/apps/org.kuchisofts.VLCRedesign.png \
                 48x48/apps/org.kuchisofts.VLCRedesign.png; do
            [ -s "/usr/share/icons/hicolor/$s" ] || fail "icon missing: $s"
        done
        cp /usr/share/icons/hicolor/256x256/apps/org.kuchisofts.VLCRedesign.png "$out/icon-256.png"
        pass "icons in hicolor (icon-256.png)" ;;
    appimage)
        desktop-file-validate "$root/org.kuchisofts.VLCRedesign.desktop" > "$out/desktop.log" 2>&1 \
            && pass "desktop entry valid" || fail "desktop entry (desktop.log)"
        cp "$root/org.kuchisofts.VLCRedesign.png" "$out/icon-256.png" && pass "AppImage icon" ;;
    flatpak)
        e=$(flatpak info --show-location org.kuchisofts.VLCRedesign)/export/share
        [ -f "$e/applications/org.kuchisofts.VLCRedesign.desktop" ] && pass "desktop entry exported" \
            || fail "no exported desktop entry"
        [ -f "$e/icons/hicolor/scalable/apps/org.kuchisofts.VLCRedesign.svg" ] && pass "icon exported" \
            || fail "no exported icon" ;;
esac

# --- run as a user ----------------------------------------------------------
# (a name no online search can take for a film's: the Hebrew file next to it
# is the subtitle)
cp /media/clip.mp4 /home/tester/zq7731.mp4
cp /media/clip.he.srt /home/tester/zq7731.he.srt
chown tester: /home/tester/zq7731.*
cp /harness/linux-pkg-tour.txt /tmp/tour.txt
chmod 644 /tmp/tour.txt
cat > /tmp/run.sh <<'EOF'
#!/bin/bash
# as the user: the version, the tour on Xvfb, a start on Weston
set -u
vlc=$1 out=$2 kind=$3
export HOME=/home/tester XDG_RUNTIME_DIR=/tmp/xdg-tester
mkdir -p "$XDG_RUNTIME_DIR" && chmod 700 "$XDG_RUNTIME_DIR"
cd "$HOME"
"$vlc" --version > "$out/version.txt" 2>&1
grep -q 'Compiled by KuchiSofts' "$out/version.txt" && echo "PASS --version: $(head -n 1 "$out/version.txt"); Compiled by KuchiSofts" \
    || echo "FAIL --version (version.txt)"
eval "$(dbus-launch --sh-syntax)"
Xvfb :99 -screen 0 1280x800x24 -nolisten tcp > "$out/xvfb.log" 2>&1 &
xvfb=$!
sleep 2
export DISPLAY=:99 QT_QPA_PLATFORM=xcb
opts="--aout=dummy --no-qt-privacy-ask -vv"
pid=
run=0
step=0
# launch [VAR=VALUE...]: VLC started (its log vlc-x11-N.log); quit: Ctrl+Q,
# and it must end within 20 s
while read -r cmd a b; do
    case "$cmd" in
        ''|'#'*) continue ;;
        launch) run=$((run + 1))
                env LANG=en_US.UTF-8 $a $b "$vlc" $opts > "$out/vlc-x11-$run.log" 2>&1 &
                pid=$! ;;
        sleep) sleep "$a" ;;
        click) xdotool mousemove "$a" "$b" click 1 ;;
        key) xdotool key --delay 80 $a ;;
        type) xdotool type --delay 60 "$a${b:+ $b}" ;;
        shot) step=$((step + 1))
              import -window root "$out/$(printf %02d $step)-$a.png" 2> /dev/null ;;
        border) # the window's 1-px frame (#4C4A48 in both themes): left edge, top edge
                f=$(ls "$out"/[0-9]*.png | tail -n 1)
                echo "INFO border of $(basename "$f"): $(convert "$f" -format '%[hex:p{128,400}] %[hex:p{640,80}]' info:)" ;;
        quit) xdotool key ctrl+q
              for i in $(seq 20); do kill -0 $pid 2>/dev/null || break; sleep 1; done
              if kill -0 $pid 2>/dev/null; then
                  echo "FAIL X11 run $run: VLC did not quit on Ctrl+Q"; kill -9 $pid
              else
                  wait $pid; echo "PASS X11 run $run: quit with Ctrl+Q (status $?)"
              fi ;;
    esac
done < /tmp/tour.txt
kill $xvfb 2>/dev/null
# Wayland: headless Weston, the clip played to 15 s, then quit
if command -v weston > /dev/null && [ "$kind" != flatpak ]; then
    # (--debug: the screenshooter protocol, for the capture of the window
    # and its border; pixman: the default renderer of the headless backend
    # draws nothing, and the screenshooter then aborted)
    weston --backend=headless --renderer=pixman --width=1280 --height=800 --socket=wayland-test --idle-time=0 \
        --debug > "$out/weston.log" 2>&1 &
    wpid=$!
    sleep 3
    # (Qt's software renderer: the container has no EGL for Wayland, and
    # without it the window did not show; a PC with a GPU has EGL)
    WAYLAND_DISPLAY=wayland-test QT_QPA_PLATFORM=wayland QT_QUICK_BACKEND=software DISPLAY= \
        timeout 40 "$vlc" $opts \
        --play-and-exit --stop-time=15 "$HOME/zq7731.mp4" > "$out/vlc-wayland.log" 2>&1 &
    vp=$!
    sleep 9
    if command -v weston-screenshooter > /dev/null; then
        (cd /tmp && rm -f wayland-screenshot*.png && WAYLAND_DISPLAY=wayland-test weston-screenshooter \
            > /dev/null 2>&1; f=$(ls wayland-screenshot*.png 2>/dev/null | head -n 1); \
            [ -n "$f" ] && mv "$f" "$out/80-wayland-player.png")
    fi
    wait $vp
    s=$?
    grep -q 'qt.qpa.wayland\|wayland' "$out/vlc-wayland.log" > /dev/null
    if grep -q 'Failed to initialize graphics backend' "$out/vlc-wayland.log"; then
        echo "FAIL Wayland: the interface could not draw (vlc-wayland.log)"
    elif [ $s -eq 0 ]; then echo "PASS Wayland: played 15 s on headless Weston and quit"
    else echo "FAIL Wayland: status $s (vlc-wayland.log)"; fi
    kill $wpid 2>/dev/null
else
    echo "INFO Wayland not run"
fi
EOF
chmod 755 /tmp/run.sh
su tester -s /bin/bash -c "/tmp/run.sh '$vlc' '$out' '$kind'" 2>&1 | tee -a "$summary"
for log in "$out"/vlc-x11-*.log "$out"/vlc-wayland.log; do
    [ -f "$log" ] || continue
    grep -E '\.qml:[0-9]+|QML|qml:' "$log" | grep -v -E 'debug:' > "$log.qml" || true
    if [ -s "$log.qml" ]; then fail "QML warnings in $(basename "$log") ($(wc -l < "$log.qml"))"
    else pass "no QML warning in $(basename "$log")"; fi
    # (not the VDPAU loader looking for a driver the PC hasn't: no NVIDIA
    # driver, VA-API or software decoding is used)
    grep -E 'cannot open shared object|no suitable decoder|undefined symbol' "$log" \
        | grep -v 'Failed to open VDPAU backend' > "$log.lib" || true
    [ -s "$log.lib" ] && fail "library or decoder errors in $(basename "$log")" || pass "no library error in $(basename "$log")"
done
grep -q 'iptv' "$out/vlc-x11.log" && info "Live TV plugin loaded"
grep -E 'subtitle|subsdec|srt' "$out/vlc-x11.log" | grep -i -E 'cp1255|windows-1255|charset|encoding|srt' | head -n 3 \
    | sed 's/^/INFO /' | tee -a "$summary"

# --- Live TV left out --------------------------------------------------------
case $kind in
    deb) apt-get remove -y -q vlc-redesign-livetv > "$out/remove-tv.log" 2>&1 ;;
    rpm) dnf remove -y vlc-redesign-livetv > "$out/remove-tv.log" 2>&1 ;;
esac
if [ "$kind" = deb ] || [ "$kind" = rpm ]; then
    if [ ! -e /opt/vlc-redesign/lib/vlc/plugins/misc/libiptv_plugin.so ] && [ -x /usr/bin/vlc-redesign ]; then
        pass "Live TV removed on its own, VLC kept"
    else
        fail "Live TV removal"
    fi
    su tester -s /bin/bash -c "export HOME=/home/tester; eval \"\$(dbus-launch --sh-syntax)\"; \
        Xvfb :98 -screen 0 1280x800x24 -nolisten tcp > /dev/null 2>&1 & x=\$!; sleep 2; \
        DISPLAY=:98 /usr/bin/vlc-redesign --aout=dummy -vv > '$out/vlc-notv.log' 2>&1 & p=\$!; sleep 12; \
        DISPLAY=:98 import -window root '$out/90-home-without-tv.png'; DISPLAY=:98 xdotool key ctrl+q; \
        sleep 5; kill \$p \$x 2>/dev/null; true"
    grep -i -E 'iptv.*(error|cannot)|\.qml:[0-9]+' "$out/vlc-notv.log" > "$out/vlc-notv.log.qml" || true
    [ -s "$out/vlc-notv.log.qml" ] && fail "errors without Live TV (vlc-notv.log.qml)" \
        || pass "starts without Live TV, no QML warning"
fi

# --- uninstall ---------------------------------------------------------------
case $kind in
    deb) apt-get purge -y -q --autoremove vlc-redesign vlc-redesign-livetv > "$out/remove.log" 2>&1 ;;
    rpm) dnf remove -y vlc-redesign vlc-redesign-livetv > "$out/remove.log" 2>&1 ;;
    appimage) rm -f /opt/test.AppImage /usr/local/bin/vlc-test; rm -rf /tmp/squashfs-root ;;
    tarball) rm -rf /opt/vlc-redesign ;;
    flatpak) flatpak uninstall --system -y --noninteractive --delete-data org.kuchisofts.VLCRedesign \
                 > "$out/remove.log" 2>&1; rm -f /usr/local/bin/vlc-test ;;
esac
snapshot > /tmp/after.txt
comm -13 /tmp/before.txt /tmp/after.txt > "$out/left-behind.txt"
# (the Flatpak runtime stays for other apps; the user's own settings are theirs)
grep -v -E '^/var/lib/flatpak/(runtime|repo)|^/var/lib/flatpak/(\.changed|appstream|exports)' "$out/left-behind.txt" \
    > "$out/left-behind-pkg.txt" || true
if [ -s "$out/left-behind-pkg.txt" ]; then fail "left behind: $(head -n 8 "$out/left-behind-pkg.txt" | tr '\n' ' ')"
else pass "removed: nothing left behind outside the user's folder"; fi
ls -A /home/tester/.config /home/tester/.local/share 2>/dev/null | sed 's/^/INFO user files kept: /' | head -n 5 | tee -a "$summary"
grep -c '^PASS' "$summary" | sed 's/^/INFO passed: /'
[ $failed -eq 0 ] && grep -q -v '^FAIL' "$summary" && ! grep -q '^FAIL' "$summary"
