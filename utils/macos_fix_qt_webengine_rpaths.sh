#!/usr/bin/env bash
#
# Two linkage bugs leave QWebEngineView blank on machines without Homebrew Qt:
#
# 1. Homebrew's split Qt kegs link QtWebEngineProcess to absolute
#    /opt/homebrew/opt/qt*/lib/... paths. Those paths are missing for end users,
#    so the helper never starts.
#
# 2. macdeployqt rewrites dylib dependencies to
#    @executable_path/../Frameworks/<lib>. That token is resolved against the
#    process executable. It is correct for Contents/MacOS/Pdc, and wrong for
#    QtWebEngineProcess, which lives inside QtWebEngineCore.framework. dyld then
#    looks for libicui18n (and the rest) inside the helper bundle, aborts the
#    helper, and the window stays empty.
#
# Rewrite helper absolute Qt paths to @rpath, and rewrite
# @executable_path/../Frameworks/ load commands on every Mach-O except the
# outer Contents/MacOS executables to @rpath/. Give Pdc and the helper an
# rpath that points at Contents/Frameworks.
#
# Usage:
#   macos_fix_qt_webengine_rpaths.sh <Pdc.app>
#
set -euo pipefail

die() { echo "ERROR: $*" >&2; exit 1; }
note() { echo "==> $*"; }

APP="${1:?usage: $0 <App.app>}"
[ -d "$APP/Contents" ] || die "not a bundle: $APP"

HELPER="$(find "$APP/Contents" -type f -path '*/QtWebEngineProcess.app/Contents/MacOS/QtWebEngineProcess' | head -1 || true)"
[ -n "$HELPER" ] || die "QtWebEngineProcess not found under $APP"

# install_name_tool invalidates an existing signature and warns. Drop it
# first. The GUI job signs the bundle afterwards.
note "removing signatures before install_name_tool"
while IFS= read -r bin; do
  [ -f "$bin" ] || continue
  file "$bin" 2>/dev/null | grep -q 'Mach-O' || continue
  codesign --remove-signature "$bin" 2>/dev/null || true
done < <(find "$APP/Contents" -type f)

# LC_LOAD_* names only. Skip LC_ID_DYLIB: that install name is not a runtime
# search path, and install_name_tool -change does not rewrite it.
list_linked_dylibs() {
  otool -l "$1" | awk '
    $1 == "cmd" && ($2 == "LC_LOAD_DYLIB" || $2 == "LC_LOAD_WEAK_DYLIB" || $2 == "LC_REEXPORT_DYLIB" || $2 == "LC_LAZY_LOAD_DYLIB") { grab=1; next }
    grab && $1 == "name" { print $2; grab=0 }
  '
}

list_rpaths() {
  otool -l "$1" | awk '/cmd LC_RPATH/{getline; getline; print $2}'
}

# Do not use `grep -q` in a pipefail pipeline: grep exits at the first match,
# awk gets SIGPIPE, and the pipeline looks like a miss.
has_rpath() {
  local bin="$1" want="$2" rp
  while IFS= read -r rp; do
    [ "$rp" = "$want" ] && return 0
  done < <(list_rpaths "$bin")
  return 1
}

is_outer_executable() {
  [ "$(dirname "$1")" = "$APP/Contents/MacOS" ]
}

note "fixing absolute Homebrew Qt paths in $HELPER"
while IFS= read -r dep; do
  case "$dep" in
    /opt/homebrew/*|/usr/local/*)
      # /opt/homebrew/opt/qtbase/lib/QtCore.framework/Versions/A/QtCore
      # → @rpath/QtCore.framework/Versions/A/QtCore
      fw="$(printf '%s\n' "$dep" | sed -n 's|.*/\(Qt[^/]*\.framework/Versions/A/[^/]*\)$|\1|p')"
      if [ -z "$fw" ]; then
        echo "WARNING: unhandled absolute dep on helper: $dep" >&2
        continue
      fi
      new="@rpath/$fw"
      note "  $dep"
      note "    -> $new"
      install_name_tool -change "$dep" "$new" "$HELPER"
      ;;
  esac
done < <(list_linked_dylibs "$HELPER")

# Ensure Frameworks is on the helper rpath (macdeployqt usually adds this).
# From QtWebEngineProcess: MacOS/Contents/app/Helpers/A/Versions/framework = 7 levels.
if ! has_rpath "$HELPER" '@loader_path/../../../../../../../'; then
  note "adding helper rpath to Contents/Frameworks"
  install_name_tool -add_rpath '@loader_path/../../../../../../../' "$HELPER"
fi

note "rewriting @executable_path/../Frameworks loads to @rpath (nested binaries)"
while IFS= read -r bin; do
  [ -f "$bin" ] || continue
  file "$bin" 2>/dev/null | grep -q 'Mach-O' || continue
  if is_outer_executable "$bin"; then
    continue
  fi
  while IFS= read -r dep; do
    case "$dep" in
      @executable_path/../Frameworks/*)
        new="@rpath/${dep#@executable_path/../Frameworks/}"
        note "  $(basename "$bin"): ${dep} -> ${new}"
        install_name_tool -change "$dep" "$new" "$bin"
        ;;
    esac
  done < <(list_linked_dylibs "$bin")
done < <(find "$APP/Contents" -type f)

MAIN="$APP/Contents/MacOS/Pdc"
if [ -f "$MAIN" ]; then
  if ! has_rpath "$MAIN" '@executable_path/../Frameworks'; then
    note "adding @executable_path/../Frameworks rpath to Pdc"
    install_name_tool -add_rpath '@executable_path/../Frameworks' "$MAIN"
  fi
fi

note "dropping Homebrew rpaths from bundled Mach-Os"
while IFS= read -r bin; do
  [ -f "$bin" ] || continue
  file "$bin" 2>/dev/null | grep -q 'Mach-O' || continue
  while IFS= read -r rp; do
    case "$rp" in
      /opt/homebrew/*|/usr/local/*|/Users/*)
        note "  -delete_rpath $rp  ($bin)"
        install_name_tool -delete_rpath "$rp" "$bin" 2>/dev/null || true
        ;;
    esac
  done < <(list_rpaths "$bin")
done < <(find "$APP/Contents" \( -name 'Pdc' -o -name 'QtWebEngineProcess' -o -name '*.dylib' \) -type f)

note "verifying QtWebEngineProcess has no absolute Homebrew deps"
if list_linked_dylibs "$HELPER" | grep -E '/opt/homebrew/|/usr/local/opt/'; then
  echo "ERROR: QtWebEngineProcess still has absolute Homebrew linkage:" >&2
  otool -L "$HELPER" >&2
  exit 1
fi

note "verifying nested binaries do not load via @executable_path/../Frameworks"
fail=0
while IFS= read -r bin; do
  [ -f "$bin" ] || continue
  file "$bin" 2>/dev/null | grep -q 'Mach-O' || continue
  if is_outer_executable "$bin"; then
    continue
  fi
  bad="$(list_linked_dylibs "$bin" | grep '@executable_path/../Frameworks/' || true)"
  if [ -n "$bad" ]; then
    echo "ERROR: $bin still loads:" >&2
    echo "$bad" >&2
    fail=1
  fi
done < <(find "$APP/Contents" -type f)
[ "$fail" -eq 0 ] || die "nested @executable_path/../Frameworks loads remain"

QTCORE="$(find "$APP/Contents/Frameworks/QtCore.framework" -type f -name QtCore | head -1 || true)"
if [ -n "$QTCORE" ]; then
  icu_name="$(list_linked_dylibs "$QTCORE" | grep '@rpath/libicui18n' | head -1 || true)"
  if [ -n "$icu_name" ]; then
    icu_base="$(basename "$icu_name")"
    [ -f "$APP/Contents/Frameworks/$icu_base" ] || die "QtCore needs $icu_base but it is not in Contents/Frameworks"
  fi
  if list_linked_dylibs "$QTCORE" | grep '@executable_path/' >/dev/null; then
    die "QtCore still has @executable_path dependencies"
  fi
fi

note "Qt WebEngine rpaths fixed"
