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
        # a release tag: its upload is everything since the release before
        # it (main may be pushed already, alone)
        refs/tags/v*)
            prev=$(git describe --tags --abbrev=0 --match 'v[0-9]*' "$local_sha^{commit}^" 2>/dev/null)
            python "$here/release-check.py" --release "${remote_ref#refs/tags/v}" ${prev:+--base "$prev"} || exit 1 ;;
        refs/heads/*) python "$here/release-check.py" || exit 1 ;;
    esac
done || {
    echo "pre-push: the project is not up to date for this upload (see above); nothing was pushed" >&2
    exit 1
}
exit 0
