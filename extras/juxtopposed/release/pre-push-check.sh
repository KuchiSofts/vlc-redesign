#!/bin/sh
# Run by the pre-push hook with the remote URL and the hook's input: a push
# to the public repository (not the private test one) passes only when
# release-check.py finds the whole project up to date (a vX.Y.Z tag: the
# release of X.Y.Z). Install: release-check-install.sh.
url="$1"
input="$2"
case "$url" in
    *KuchiSofts/vlc-redesign.git|*KuchiSofts/vlc-redesign) ;;
    *) exit 0 ;;
esac
here="$(dirname "$0")"
zero=0000000000000000000000000000000000000000
echo "$input" | while read -r local_ref local_sha remote_ref remote_sha; do
    [ -z "$local_sha" ] && continue
    [ "$local_sha" = "$zero" ] && continue
    case "$remote_ref" in
        refs/tags/v*) python "$here/release-check.py" --release "${remote_ref#refs/tags/v}" || exit 1 ;;
        refs/heads/*) python "$here/release-check.py" || exit 1 ;;
    esac
done || {
    echo "pre-push: the project is not up to date for this upload (see above); nothing was pushed" >&2
    exit 1
}
exit 0
