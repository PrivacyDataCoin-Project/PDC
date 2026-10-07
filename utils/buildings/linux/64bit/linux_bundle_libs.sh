#!/bin/bash
# Bundle private shared libraries into a Linux GUI tree built on Ubuntu 22.04.
# glibc and libstdc++ stay on the host so the same archive runs on 22.04 and newer.
# OpenGL and the core X11 client libraries also stay on the host.
set -euo pipefail

pdc_is_host_lib() {
  case "$(basename "$1")" in
    linux-vdso.so*|ld-linux*.so*|libc.so*|libm.so*|libdl.so*|libpthread.so*|librt.so*|libresolv.so*|libutil.so*|libnsl.so*|libgcc_s.so*|libstdc++.so*)
      return 0
      ;;
    libGL.so*|libGLdispatch.so*|libGLX.so*|libOpenGL.so*|libEGL.so*|libGLESv2.so*|libdrm.so*|libgbm.so*|libvulkan.so*)
      return 0
      ;;
    libsystemd.so*|libudev.so*|libselinux.so*|libmount.so*|libblkid.so*)
      return 0
      ;;
    libX11.so*|libX11-xcb.so*|libxcb.so|libxcb.so.*|libXext.so*|libXau.so*|libXdmcp.so*)
      return 0
      ;;
    libglib-2.0.so*|libgobject-2.0.so*|libgio-2.0.so*|libgmodule-2.0.so*|libgthread-2.0.so*)
      return 0
      ;;
    libfontconfig.so*|libfreetype.so*|libdbus-1.so*|libz.so*|libexpat.so*|libpcre2-8.so*|libffi.so*|libbz2.so*|liblzma.so*|libzstd.so*|libpng16.so*)
      return 0
      ;;
    libharfbuzz.so*|libgraphite2.so*|libbrotlidec.so*|libbrotlicommon.so*|libbsd.so*|libmd.so*|libasound.so*|libpango-1.0.so*|libpangocairo-1.0.so*|libpangoft2-1.0.so*|libatk-1.0.so*|libcups.so*)
      return 0
      ;;
  esac
  return 1
}

pdc_patchelf_rpath() {
  local file="$1"
  local rpath="$2"
  if [[ ! -f "$file" || -L "$file" ]]; then
    return 0
  fi
  if ! file -b "$file" | grep -q ELF; then
    return 0
  fi
  patchelf --set-rpath "$rpath" "$file"
}

pdc_materialize_symlinks() {
  local libdir="$1"
  local link real base realbase tmp
  shopt -s nullglob
  for link in "$libdir"/* "$libdir"/nss/*; do
    [[ -L "$link" ]] || continue
    real=$(readlink -f "$link" || true)
    [[ -f "$real" ]] || continue
    case "$real" in
      "$libdir"/*) continue ;;
    esac
    base=$(basename "$link")
    realbase=$(basename "$real")
    tmp=$(mktemp)
    cp -a "$real" "$tmp"
    rm -f "$link"
    if [[ "$base" == "$realbase" ]]; then
      mv "$tmp" "$libdir/$realbase"
    else
      if [[ ! -f "$libdir/$realbase" ]]; then
        mv "$tmp" "$libdir/$realbase"
      else
        rm -f "$tmp"
      fi
      ln -sfn "$realbase" "$libdir/$base"
    fi
  done
  shopt -u nullglob
}

pdc_copy_real_lib() {
  local libdir="$1"
  local path="$2"
  local soname="$3"
  local real realbase
  [[ -e "$path" ]] || return 0
  pdc_is_host_lib "$soname" && return 0
  real=$(readlink -f "$path")
  [[ -f "$real" ]] || return 0
  realbase=$(basename "$real")
  if [[ ! -f "$libdir/$realbase" ]]; then
    cp -a "$real" "$libdir/$realbase"
  fi
  if [[ "$soname" != "$realbase" && ! -e "$libdir/$soname" ]]; then
    ln -sfn "$realbase" "$libdir/$soname"
  fi
}

pdc_find_system_lib() {
  local name="$1"
  local found
  found=$(ldconfig -p 2>/dev/null | awk -v n="$name" '$1 == n { print $NF; exit }')
  if [[ -n "$found" && -e "$found" ]]; then
    echo "$found"
    return 0
  fi
  found=$(find /usr/lib/x86_64-linux-gnu /lib/x86_64-linux-gnu -name "$name" -print -quit 2>/dev/null || true)
  if [[ -n "$found" && -e "$found" ]]; then
    echo "$found"
  fi
}

pdc_enqueue_elf() {
  local path="$1"
  [[ -e "$path" ]] || return 0
  if [[ -n "${PDC_QUEUED[$path]:-}" ]]; then
    return 0
  fi
  PDC_QUEUED["$path"]=1
  PDC_QUEUE+=("$path")
}

pdc_bundle_from_ldd() {
  local libdir="$1"
  local obj line soname path
  local -i index=0
  while [[ $index -lt ${#PDC_QUEUE[@]} ]]; do
    obj="${PDC_QUEUE[$index]}"
    index=$((index + 1))
    if ! file -b "$obj" | grep -q ELF; then
      continue
    fi
    while IFS= read -r line; do
      [[ "$line" == *"=>"* ]] || continue
      [[ "$line" == *"not found"* ]] && continue
      soname=$(echo "$line" | awk '{ print $1 }')
      path=$(echo "$line" | awk '{ print $3 }')
      [[ -n "$soname" && -n "$path" && "$path" != "not" ]] || continue
      pdc_copy_real_lib "$libdir" "$path" "$soname"
      if [[ -e "$libdir/$soname" ]]; then
        pdc_enqueue_elf "$(readlink -f "$libdir/$soname")"
      fi
    done < <(ldd "$obj" 2>/dev/null || true)
  done
}

pdc_bundle_named_libs() {
  local libdir="$1"
  local name path
  local names=(
    libnss3.so libnssutil3.so libnspr4.so libplc4.so libplds4.so libsmime3.so libssl3.so
    libfreebl3.so libfreeblpriv3.so libsoftokn3.so libnssckbi.so
    libflac.so.8 libvpx.so.7 libwebp.so.7 libevent-2.1.so.7 libminizip.so.1 libsnappy.so.1
    libopus.so.0 libxkbcommon.so.0 libxkbcommon-x11.so.0 libxcb-xinerama.so.0 libxcb-glx.so.0
    libXcomposite.so.1 libXdamage.so.1 libXfixes.so.3 libXrandr.so.2 libXrender.so.1
    libXi.so.6 libXtst.so.6 libXss.so.1 libxshmfence.so.1
    libjpeg.so.8 libpcre.so.3 libxml2.so.2 libxslt.so.1 libpci.so.3
  )
  for name in "${names[@]}"; do
    path=$(pdc_find_system_lib "$name" || true)
    if [[ -z "$path" ]]; then
      echo "Note: optional library $name is not on this builder"
      continue
    fi
    pdc_copy_real_lib "$libdir" "$path" "$name"
    if [[ -e "$libdir/$name" ]]; then
      pdc_enqueue_elf "$(readlink -f "$libdir/$name")"
    fi
  done
}

pdc_set_bundle_rpath() {
  local pkg="$1"
  local file
  shopt -s nullglob
  for file in "$pkg/Pdc" "$pkg/pdcd" "$pkg/simplewallet" "$pkg/connectivity_tool" "$pkg/QtWebEngineProcess"; do
    pdc_patchelf_rpath "$file" '$ORIGIN/lib'
  done
  for file in "$pkg"/lib/platforms/*.so "$pkg"/lib/xcbglintegrations/*.so; do
    pdc_patchelf_rpath "$file" '$ORIGIN/..'
  done
  for file in "$pkg"/lib/*.so* "$pkg"/lib/nss/*.so*; do
    pdc_patchelf_rpath "$file" '$ORIGIN'
  done
  shopt -u nullglob
}

pdc_bundle_private_libs() {
  local pkg="$1"
  local libdir="$pkg/lib"
  local so
  mkdir -p "$libdir"
  pdc_materialize_symlinks "$libdir"
  declare -gA PDC_QUEUED=()
  declare -ga PDC_QUEUE=()
  pdc_enqueue_elf "$pkg/Pdc"
  pdc_enqueue_elf "$pkg/QtWebEngineProcess"
  shopt -s nullglob
  for so in "$libdir"/*.so* "$libdir"/platforms/*.so "$libdir"/xcbglintegrations/*.so; do
    [[ -e "$so" ]] || continue
    pdc_enqueue_elf "$(readlink -f "$so")"
  done
  shopt -u nullglob
  pdc_bundle_from_ldd "$libdir"
  pdc_bundle_named_libs "$libdir"
  pdc_bundle_from_ldd "$libdir"
  pdc_materialize_symlinks "$libdir"
  if ! command -v patchelf >/dev/null; then
    echo "ERROR: patchelf is required to point the GUI at its bundled lib/"
    exit 1
  fi
  pdc_set_bundle_rpath "$pkg"
}

pdc_check_one() {
  local libdir="$1"
  local obj="$2"
  local line soname failed=0
  [[ -e "$obj" ]] || return 0
  while IFS= read -r line; do
    [[ "$line" == *"not found"* ]] || continue
    soname=$(echo "$line" | awk '{ print $1 }')
    echo "ERROR: $obj needs $soname and it is not in the bundle or the host baseline"
    failed=1
  done < <(LD_LIBRARY_PATH="$libdir" ldd "$obj" 2>/dev/null || true)
  return "$failed"
}

pdc_assert_bundled() {
  local libdir="$1"
  local obj="$2"
  local soname="$3"
  local resolved
  resolved=$(LD_LIBRARY_PATH="$libdir" ldd "$obj" | awk -v s="$soname" '$1 == s { print $3; exit }')
  case "$resolved" in
    "$libdir"/*) ;;
    *)
      echo "ERROR: $soname for $(basename "$obj") resolved to '${resolved:-missing}', expected under $libdir"
      return 1
      ;;
  esac
}

pdc_check_private_libs() {
  local pkg="$1"
  local libdir="$pkg/lib"
  local obj failed=0
  shopt -s nullglob
  local objects=("$pkg/Pdc" "$pkg/QtWebEngineProcess" "$pkg/pdcd" "$pkg/simplewallet" "$pkg/connectivity_tool")
  objects+=("$libdir"/platforms/*.so "$libdir"/xcbglintegrations/*.so "$libdir"/libQt5*.so*)
  shopt -u nullglob
  for obj in "${objects[@]}"; do
    [[ -e "$obj" ]] || continue
    pdc_check_one "$libdir" "$obj" || failed=1
  done
  pdc_assert_bundled "$libdir" "$pkg/Pdc" "libssl.so.1.1" || failed=1
  pdc_assert_bundled "$libdir" "$pkg/Pdc" "libcrypto.so.1.1" || failed=1
  if [[ -e "$libdir/libQt5Core.so.5" ]]; then
    pdc_assert_bundled "$libdir" "$libdir/libQt5Core.so.5" "libicuuc.so.56" || failed=1
  fi
  if [[ "$failed" -ne 0 ]]; then
    exit 1
  fi
  echo "Portable library check passed for $pkg"
}

usage() {
  echo "Usage: $0 bundle|check <package-dir>" >&2
  exit 2
}

main() {
  local cmd="${1:-}"
  local pkg="${2:-}"
  [[ -n "$cmd" && -d "$pkg" ]] || usage
  case "$cmd" in
    bundle) pdc_bundle_private_libs "$pkg" ;;
    check) pdc_check_private_libs "$pkg" ;;
    *) usage ;;
  esac
}

main "$@"
