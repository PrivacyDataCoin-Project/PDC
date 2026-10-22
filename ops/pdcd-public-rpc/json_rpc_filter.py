#!/usr/bin/env python3
"""Allowlist filter for public pdcd /json_rpc (wallet sync/read).

Transfers use HTTP /sendrawtransaction (nginx allowlist), not this filter.
Mining / destructive JSON-RPC methods return HTTP 403.
Upstream is the full local daemon (127.0.0.1:19211).
"""

from __future__ import annotations

import json
import os
import sys
import urllib.error
import urllib.request
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer

UPSTREAM = os.environ.get("PDCD_RPC_UPSTREAM", "http://127.0.0.1:19211").rstrip("/")
LISTEN_HOST = os.environ.get("PDCD_RPC_FILTER_HOST", "127.0.0.1")
LISTEN_PORT = int(os.environ.get("PDCD_RPC_FILTER_PORT", "19215"))
MAX_BODY = int(os.environ.get("PDCD_RPC_FILTER_MAX_BODY", str(16 * 1024 * 1024)))

# Wallet sync / light-client reads (wallet2 + iOS remote-node).
ALLOWED_JSON_RPC = frozenset(
    {
        "getinfo",
        "getblockcount",
        "on_getblockhash",
        "getlastblockheader",
        "getblockheaderbyhash",
        "getblockheaderbyheight",
        "get_alias_details",
        "get_alias_by_address",
        "get_alias_reward",
        "get_est_height_from_date",
        "find_outs_in_recent_blocks",
        "get_blocks_details",
        "get_tx_details",
        "search_by_id",
        "get_out_info",
        "get_multisig_info",
        "get_all_alias_details",
        "get_aliases",
        "get_pool_txs_details",
        "get_pool_txs_brief_details",
        "get_all_pool_tx_list",
        "get_pool_info",
        "getrandom_outs",
        "getrandom_outs1",
        "getrandom_outs3",
        "get_votes",
        "get_asset_info",
        "get_assets_list",
        "decrypt_tx_details",
        "get_main_block_details",
        "get_alt_block_details",
        "get_alt_blocks_details",
        "get_current_core_tx_expiration_median",
        "marketplace_global_get_offers_ex",
        "validate_signature",
    }
)

DENIED_JSON_RPC = frozenset(
    {
        "getblocktemplate",
        "submitblock",
        "submitblock2",
        "reset_transaction_pool",
        "remove_tx_from_pool",
    }
)


class Handler(BaseHTTPRequestHandler):
    protocol_version = "HTTP/1.1"

    def log_message(self, fmt: str, *args) -> None:
        sys.stderr.write("%s - %s\n" % (self.address_string(), fmt % args))

    def _read_body(self) -> bytes:
        length = int(self.headers.get("Content-Length", "0") or "0")
        if length < 0 or length > MAX_BODY:
            return b""
        return self.rfile.read(length) if length else b""

    def _send(self, code: int, body: bytes, content_type: str = "application/json") -> None:
        self.send_response(code)
        self.send_header("Content-Type", content_type)
        self.send_header("Content-Length", str(len(body)))
        self.send_header("Connection", "close")
        self.end_headers()
        if body:
            self.wfile.write(body)

    def _forbid(self, reason: str) -> None:
        payload = json.dumps(
            {
                "jsonrpc": "2.0",
                "id": None,
                "error": {"code": -32000, "message": f"forbidden: {reason}"},
            }
        ).encode()
        self._send(403, payload)

    def do_POST(self) -> None:  # noqa: N802
        path = self.path.split("?", 1)[0]
        if path not in ("/json_rpc", "/json_rpc/"):
            self._forbid("path")
            return

        raw = self._read_body()
        if not raw:
            self._forbid("empty body")
            return

        try:
            req = json.loads(raw.decode("utf-8", errors="replace"))
        except json.JSONDecodeError:
            self._forbid("invalid json")
            return

        if isinstance(req, list):
            self._forbid("batch not allowed")
            return
        if not isinstance(req, dict):
            self._forbid("invalid request")
            return

        method = req.get("method")
        if not isinstance(method, str):
            self._forbid("missing method")
            return
        if method in DENIED_JSON_RPC or method not in ALLOWED_JSON_RPC:
            self._forbid(f"method {method}")
            return

        upstream = f"{UPSTREAM}/json_rpc"
        try:
            ureq = urllib.request.Request(
                upstream,
                data=raw,
                headers={
                    "Content-Type": self.headers.get("Content-Type", "application/json"),
                    "Accept": self.headers.get("Accept", "application/json"),
                },
                method="POST",
            )
            with urllib.request.urlopen(ureq, timeout=120) as resp:
                body = resp.read()
                self._send(resp.status, body, resp.headers.get("Content-Type", "application/json"))
        except urllib.error.HTTPError as e:
            body = e.read() if e.fp else b""
            self._send(e.code, body or b'{"error":"upstream http error"}')
        except Exception as e:  # noqa: BLE001
            self._send(502, json.dumps({"error": f"upstream: {e}"}).encode())

    def do_GET(self) -> None:  # noqa: N802
        self._forbid("GET not allowed")


def main() -> None:
    server = ThreadingHTTPServer((LISTEN_HOST, LISTEN_PORT), Handler)
    print(f"pdcd json_rpc filter on {LISTEN_HOST}:{LISTEN_PORT} -> {UPSTREAM}", flush=True)
    server.serve_forever()


if __name__ == "__main__":
    main()
