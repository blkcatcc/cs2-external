#!/usr/bin/env bash
set -euo pipefail

MOD="blkcat"
SELF="$(readlink -f "$0")"
ROOT="$(cd "$(dirname "$SELF")" && pwd)"
KO="$ROOT/kernel/$MOD.ko"
DEV="/dev/$MOD"

usage() {
	echo "usage: $(basename "$0") {load|unload|reload|status}" >&2
	exit 2
}

loaded() { lsmod | grep -qw "$MOD"; }

cmd="${1:-}"
case "$cmd" in
	load|unload|reload|status) ;;
	*) usage ;;
esac

if [ "$cmd" != "status" ] && [ "$(id -u)" -ne 0 ]; then
	exec sudo "$SELF" "$@"
fi

case "$cmd" in
load)
	if loaded; then
		echo "$MOD: already loaded"
		exit 0
	fi
	[ -f "$KO" ] || make -C "$ROOT/kernel"
	insmod "$KO"
	[ -e "$DEV" ] || echo "warning: $DEV missing, check dmesg" >&2
	echo "$MOD: loaded"
	;;
unload)
	if ! loaded; then
		echo "$MOD: not loaded"
		exit 0
	fi
	rmmod "$MOD"
	echo "$MOD: unloaded"
	;;
reload)
	"$SELF" unload
	"$SELF" load
	;;
status)
	if loaded; then
		echo "$MOD: loaded ($(ls -l "$DEV" 2>/dev/null || echo "no $DEV"))"
	else
		echo "$MOD: not loaded"
	fi
	;;
esac
