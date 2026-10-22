#!/usr/bin/env bash
# Copy a built pdcd into the shadow path — never touches live /home/ubuntu/pdc/pdcd.
# No GitHub release required (CI artifact, local cmake build, or rsync).
set -euo pipefail

SRC="${1:-}"
SHADOW_DIR="${SHADOW_DIR:-/home/ubuntu/pdc-shadow}"
DEST="${SHADOW_BIN:-${SHADOW_DIR}/pdcd}"

if [[ -z "$SRC" || ! -f "$SRC" ]]; then
  echo "usage: $0 /path/to/pdcd" >&2
  exit 2
fi
if [[ "$(readlink -f "$SRC")" == "$(readlink -f /home/ubuntu/pdc/pdcd 2>/dev/null || true)" ]]; then
  echo "REFUSE: source is the live binary path" >&2
  exit 1
fi
if [[ "$(readlink -f "$DEST" 2>/dev/null || true)" == "$(readlink -f /home/ubuntu/pdc/pdcd 2>/dev/null || true)" ]]; then
  echo "REFUSE: DEST resolves to live binary" >&2
  exit 1
fi

mkdir -p "$SHADOW_DIR"
install -m 0755 "$SRC" "$DEST"
echo "installed shadow binary → $DEST"
"$DEST" --version 2>/dev/null || "$DEST" --help 2>&1 | head -3 || true
echo "next: ./ops/pdcx-oracle-fee/run-shadow-pdcd.sh preflight && … start"
