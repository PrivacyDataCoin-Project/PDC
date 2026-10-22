#!/usr/bin/env bash
# Local / loopback smoke: tip hash must be present without expensive getinfo flags.
set -euo pipefail

RPC_URL="${RPC_URL:-http://127.0.0.1:19211}"

echo "== HTTP /getinfo =="
HTTP_JSON="$(curl -fsS "${RPC_URL}/getinfo")"
echo "$HTTP_JSON" | python3 -c '
import json,sys
d=json.load(sys.stdin)
h=d.get("last_block_hash") or ""
print("height=", d.get("height"))
print("last_block_hash=", h[:16]+("…" if len(h)>16 else ""), "len=", len(h))
if not h or len(h)!=64:
  raise SystemExit("FAIL: last_block_hash missing/invalid on /getinfo")
print("OK http getinfo tip hash")
'

echo "== JSON-RPC getinfo (empty params) =="
RPC_JSON="$(curl -fsS -X POST "${RPC_URL}/json_rpc" \
  -H "Content-Type: application/json" \
  -d "{\"jsonrpc\":\"2.0\",\"id\":0,\"method\":\"getinfo\",\"params\":{}}")"
echo "$RPC_JSON" | python3 -c '
import json,sys
r=json.load(sys.stdin).get("result") or {}
h=r.get("last_block_hash") or ""
print("height=", r.get("height"))
print("last_block_hash=", h[:16]+("…" if len(h)>16 else ""), "len=", len(h))
if not h or len(h)!=64:
  raise SystemExit("FAIL: last_block_hash missing/invalid on json_rpc getinfo")
print("OK json_rpc getinfo tip hash")
'

echo "ALL TIP-HASH SMOKES PASSED"
