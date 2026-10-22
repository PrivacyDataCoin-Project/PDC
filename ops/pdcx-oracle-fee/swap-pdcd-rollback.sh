#!/usr/bin/env bash
# Rollback-friendly pdcd binary swap on mainnet host.
# Usage:
#   NEW_BIN=/path/to/pdcd.new ./swap-pdcd-rollback.sh install
#   ./swap-pdcd-rollback.sh rollback
set -euo pipefail

PDC_DIR="${PDC_DIR:-/home/ubuntu/pdc}"
LIVE="${PDC_DIR}/pdcd"
PREV="${PDC_DIR}/pdcd.prev"
STAMP="$(date -u +%Y%m%dT%H%M%SZ)"

cmd="${1:-}"
case "$cmd" in
  install)
    NEW_BIN="${NEW_BIN:?set NEW_BIN to the candidate pdcd path}"
    test -x "$NEW_BIN"
    sudo systemctl stop pdcd
    cp -a "$LIVE" "${PREV}.${STAMP}"
    cp -a "$LIVE" "$PREV"
    install -m 0755 "$NEW_BIN" "$LIVE"
    sudo systemctl start pdcd
    sleep 3
    curl -fsS -X POST http://127.0.0.1:19211/json_rpc \
      -H 'Content-Type: application/json' \
      -d '{"jsonrpc":"2.0","id":0,"method":"getinfo","params":{}}' | head -c 400
    echo
    echo "OK installed. Rollback: $0 rollback"
    ;;
  rollback)
    test -x "$PREV"
    sudo systemctl stop pdcd
    cp -a "$PREV" "$LIVE"
    sudo systemctl start pdcd
    sleep 3
    curl -fsS http://127.0.0.1:19211/getinfo | head -c 200
    echo
    echo "OK rolled back to $PREV"
    ;;
  *)
    echo "usage: $0 install|rollback" >&2
    exit 2
    ;;
esac
