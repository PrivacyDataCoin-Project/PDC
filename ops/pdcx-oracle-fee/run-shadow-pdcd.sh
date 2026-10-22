#!/usr/bin/env bash
# Run a mainnet shadow pdcd alongside live — never touches live binary/data/systemd.
set -euo pipefail

LIVE_RPC="${LIVE_RPC:-http://127.0.0.1:19211}"
SHADOW_DIR="${SHADOW_DIR:-/home/ubuntu/pdc-shadow}"
SHADOW_DATA="${SHADOW_DATA:-/home/ubuntu/.PDC-shadow-12}"
SHADOW_BIN="${SHADOW_BIN:-${SHADOW_DIR}/pdcd}"
SHADOW_RPC_PORT="${SHADOW_RPC_PORT:-19221}"
SHADOW_P2P_PORT="${SHADOW_P2P_PORT:-19131}"
PID_FILE="${SHADOW_DIR}/pdcd-shadow.pid"
LOG_FILE="${SHADOW_DATA}/pdcd-shadow.log"

cmd="${1:-}"

assert_not_live() {
  if [[ "$(readlink -f "$SHADOW_BIN" 2>/dev/null || true)" == "$(readlink -f /home/ubuntu/pdc/pdcd 2>/dev/null || true)" ]]; then
    echo "REFUSE: SHADOW_BIN resolves to live /home/ubuntu/pdc/pdcd" >&2
    exit 1
  fi
  if [[ "$SHADOW_DATA" == "/home/ubuntu/.PDC" ]]; then
    echo "REFUSE: SHADOW_DATA must not be live data-dir" >&2
    exit 1
  fi
  if [[ "$SHADOW_RPC_PORT" == "19211" || "$SHADOW_P2P_PORT" == "19121" ]]; then
    echo "REFUSE: shadow ports must differ from live 19211/19121" >&2
    exit 1
  fi
}

case "$cmd" in
  preflight)
    assert_not_live
    echo "live RPC: $LIVE_RPC"
    curl -fsS "$LIVE_RPC/getinfo" | python3 -c 'import json,sys;d=json.load(sys.stdin);print("live height",d.get("height"),"ok")'
    echo "shadow bin: $SHADOW_BIN exists=$([[ -x $SHADOW_BIN ]] && echo yes || echo NO)"
    echo "shadow data: $SHADOW_DATA"
    echo "shadow rpc: 127.0.0.1:$SHADOW_RPC_PORT  p2p: 127.0.0.1:$SHADOW_P2P_PORT"
    ss -lntp | grep -E ":${SHADOW_RPC_PORT}|:${SHADOW_P2P_PORT}" && echo "WARN: shadow ports already in use" || echo "shadow ports free"
    echo "PREFLIGHT OK (live untouched)"
    ;;
  start)
    assert_not_live
    test -x "$SHADOW_BIN"
    mkdir -p "$SHADOW_DIR" "$SHADOW_DATA"
    if [[ -f "$PID_FILE" ]] && kill -0 "$(cat "$PID_FILE")" 2>/dev/null; then
      echo "already running pid=$(cat "$PID_FILE")"
      exit 0
    fi
    # Sync from local live peer only; do not announce; loopback binds.
    nohup "$SHADOW_BIN" \
      --data-dir="$SHADOW_DATA" \
      --rpc-bind-ip=127.0.0.1 \
      --rpc-bind-port="$SHADOW_RPC_PORT" \
      --p2p-bind-ip=127.0.0.1 \
      --p2p-bind-port="$SHADOW_P2P_PORT" \
      --add-priority-node=127.0.0.1:19121 \
      --use-only-priority-nodes \
      --hide-my-port \
      --rpc-ignore-offline \
      --disable-upnp \
      --log-level=2 \
      --no-console \
      >>"$LOG_FILE" 2>&1 &
    echo $! >"$PID_FILE"
    echo "started shadow pid=$(cat "$PID_FILE") log=$LOG_FILE"
    echo "smoke: RPC_URL=http://127.0.0.1:${SHADOW_RPC_PORT} ./local-smoke-tip-hash.sh"
    ;;
  stop)
    assert_not_live
    if [[ -f "$PID_FILE" ]]; then
      pid=$(cat "$PID_FILE")
      kill "$pid" 2>/dev/null || true
      rm -f "$PID_FILE"
      echo "stopped shadow pid=$pid (live pdcd untouched)"
    else
      echo "no pid file"
    fi
    ;;
  status)
    curl -fsS "http://127.0.0.1:${SHADOW_RPC_PORT}/getinfo" 2>/dev/null \
      | python3 -c 'import json,sys;d=json.load(sys.stdin);print("shadow height",d.get("height"),"hash_len",len(d.get("last_block_hash") or ""),"version",d.get("version"))' \
      || echo "shadow RPC down"
    curl -fsS "$LIVE_RPC/getinfo" | python3 -c 'import json,sys;d=json.load(sys.stdin);print("live height",d.get("height"),"hash_len",len(d.get("last_block_hash") or ""))'
    ;;
  *)
    echo "usage: $0 preflight|start|stop|status" >&2
    exit 2
    ;;
esac
