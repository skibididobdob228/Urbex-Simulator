#!/usr/bin/env bash
set -u

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
project="$root/project"
godot="${GODOT:-}"
build=1
asan=0
levels=""
jobs="$(nproc 2>/dev/null || echo 4)"

usage() {
	echo "usage: tests/run_selftest.sh [--no-build] [--asan] [--levels=hospital,shelter,rooftop] [--godot=/path/to/godot]"
}

for arg in "$@"; do
	case "$arg" in
		--no-build) build=0 ;;
		--asan) asan=1 ;;
		--levels=*) levels="${arg#*=}" ;;
		--godot=*) godot="${arg#*=}" ;;
		-h | --help)
			usage
			exit 0
			;;
		*)
			usage
			exit 2
			;;
	esac
done

if [ -z "$godot" ]; then
	for candidate in godot godot4 godot-4; do
		if command -v "$candidate" >/dev/null 2>&1; then
			godot="$(command -v "$candidate")"
			break
		fi
	done
fi
if [ -z "$godot" ] || [ ! -x "$godot" ]; then
	echo "Godot not found. Install it (sudo pacman -S godot) or set GODOT=/path/to/godot" >&2
	exit 2
fi

cd "$root" || exit 2

if [ "$asan" = 1 ]; then
	scons platform=linux target=template_debug sanitize=yes -j"$jobs" || exit 2
elif [ "$build" = 1 ]; then
	scons platform=linux target=template_debug -j"$jobs" || exit 2
fi

if [ ! -f "$project/.godot/extension_list.cfg" ]; then
	echo "First import of the project..."
	"$godot" --headless --editor --path "$project" --quit-after 300 >/dev/null 2>&1
fi

preload=""
work="$(mktemp -d)"
if [ "$asan" = 1 ]; then
	cat >"$work/nodeepbind.c" <<'SHIM'
#define _GNU_SOURCE
#include <dlfcn.h>

void *dlopen(const char *file, int mode) {
	static void *(*real)(const char *, int) = 0;
	if (!real) {
		real = (void *(*)(const char *, int))dlsym(RTLD_NEXT, "dlopen");
	}
	return real(file, mode & ~RTLD_DEEPBIND);
}
SHIM
	cc -shared -fPIC -O2 -o "$work/nodeepbind.so" "$work/nodeepbind.c" -ldl || exit 2
	preload="$work/nodeepbind.so:$(cc -print-file-name=libasan.so)"
	export ASAN_OPTIONS="detect_leaks=0:verify_asan_link_order=0:halt_on_error=1"
	export UBSAN_OPTIONS="print_stacktrace=1:halt_on_error=1"
fi

selftest="--selftest"
if [ -n "$levels" ]; then
	selftest="--selftest=$levels"
fi

log="$work/selftest.log"
echo "Godot: $("$godot" --version 2>/dev/null | head -n 1)"
LD_PRELOAD="$preload" "$godot" --headless --fixed-fps 60 --path "$project" -- "$selftest" >"$log" 2>&1
code=$?

grep -E "^\[selftest\]" "$log"
problems="$(grep -E "leaked at exit|AddressSanitizer|runtime error:|handle_crash|SUMMARY:" "$log")"
if [ -n "$problems" ]; then
	echo "$problems"
	code=1
fi
if ! grep -q "^\[selftest\] PASS" "$log"; then
	[ "$code" = 0 ] && code=1
fi

if [ "$asan" = 1 ]; then
	scons platform=linux target=template_debug -j"$jobs" >/dev/null || code=2
fi

if [ "$code" = 0 ]; then
	echo "OK"
	rm -rf "$work"
else
	echo "FAILED (exit $code), full log: $log"
fi
exit "$code"
