#!/usr/bin/env python3
"""Sign website/api/assets_whitelist.json with WALLET_ASSETS_WHITELIST_VALIDATION key.

Uses cn_fast_hash + crypto_ops::generate_signature from a PDC binary (connectivity_tool).
Never prints private keys. Algorithm must match wallet2::load_whitelisted_tokens (R-02).
"""
from __future__ import annotations

import argparse
import ctypes
import json
import re
import sys
from pathlib import Path

EXPECTED_PUB = "1bfd31dc840c0f205d9b72df18d298cbdef716e4389684c7da19f3db2a77c65f"


def canonical_blob(assets: list[dict]) -> bytes:
    lines: list[str] = []
    for a in assets:
        lines.append(str(a["asset_id"]).lower())
        lines.append(str(int(a["total_max_supply"])))
        lines.append(str(int(a["current_supply"])))
        lines.append(str(int(a["decimal_point"])))
        lines.append(str(a["ticker"]))
        lines.append(str(a["full_name"]))
        lines.append(str(a.get("meta_info") or ""))
        lines.append(str(a["owner"]).lower())
        lines.append("1" if a.get("hidden_supply") else "0")
    return ("\n".join(lines) + "\n").encode("utf-8")


def load_secret_pair(secret_path: Path) -> tuple[bytes, bytes]:
    text = secret_path.read_text()
    m = re.search(
        r"WALLET_ASSETS_WHITELIST_VALIDATION\s*\n\s*public\s*=\s*([0-9a-fA-F]{64})\s*\n\s*private\s*=\s*([0-9a-fA-F]{64})",
        text,
    )
    if not m:
        raise SystemExit(f"WALLET_ASSETS_WHITELIST_VALIDATION keys not found in {secret_path}")
    pub_hex, priv_hex = m.group(1).lower(), m.group(2).lower()
    if pub_hex != EXPECTED_PUB:
        raise SystemExit(
            f"pubkey in secret file does not match currency_config "
            f"(got {pub_hex[:16]}…, expected {EXPECTED_PUB[:16]}…)"
        )
    return bytes.fromhex(pub_hex), bytes.fromhex(priv_hex)


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--json", type=Path, required=True, help="assets_whitelist.json path")
    ap.add_argument(
        "--connectivity-tool",
        type=Path,
        required=True,
        help="PDC connectivity_tool (or other binary exporting crypto symbols)",
    )
    ap.add_argument(
        "--secret",
        type=Path,
        default=None,
        help="pdc_authority_keys.SECRET.txt (default: utils/ next to this script, then ~/pdc_authority_keys.SECRET.txt)",
    )
    args = ap.parse_args()

    secret = args.secret
    if secret is None:
        candidates = [
            Path(__file__).resolve().parent / "pdc_authority_keys.SECRET.txt",
            Path.home() / "pdc_authority_keys.SECRET.txt",
        ]
        secret = next((p for p in candidates if p.is_file()), None)
        if secret is None:
            raise SystemExit("secret file not found; pass --secret")

    pub, sec = load_secret_pair(secret)
    data = json.loads(args.json.read_text())
    if "assets" not in data:
        raise SystemExit("JSON missing assets[]")

    lib = ctypes.CDLL(str(args.connectivity_tool))
    cn_fast_hash = lib.cn_fast_hash
    cn_fast_hash.argtypes = [ctypes.c_void_p, ctypes.c_size_t, ctypes.c_char_p]
    cn_fast_hash.restype = None

    sk2pk = lib._ZN6crypto10crypto_ops24secret_key_to_public_keyERKNS_10secret_keyERNS_10public_keyE
    sk2pk.argtypes = [ctypes.c_void_p, ctypes.c_void_p]
    sk2pk.restype = ctypes.c_bool

    gen_sig = lib._ZN6crypto10crypto_ops18generate_signatureERKNS_4hashERKNS_10public_keyERKNS_10secret_keyERNS_9signatureE
    gen_sig.argtypes = [ctypes.c_void_p, ctypes.c_void_p, ctypes.c_void_p, ctypes.c_void_p]
    gen_sig.restype = None

    chk_sig = lib._ZN6crypto10crypto_ops15check_signatureERKNS_4hashERKNS_10public_keyERKNS_9signatureE
    chk_sig.argtypes = [ctypes.c_void_p, ctypes.c_void_p, ctypes.c_void_p]
    chk_sig.restype = ctypes.c_bool

    pub_out = ctypes.create_string_buffer(32)
    if not sk2pk(sec, pub_out) or pub_out.raw != pub:
        raise SystemExit("private key does not derive expected public key")

    blob = canonical_blob(data["assets"])
    h = ctypes.create_string_buffer(32)
    cn_fast_hash(blob, len(blob), h)

    sig = ctypes.create_string_buffer(64)
    gen_sig(h, pub, sec, sig)
    if not chk_sig(h, pub, sig):
        raise SystemExit("self-verify failed")
    if all(b == 0 for b in sig.raw):
        raise SystemExit("signature is all zeros")

    data["signature"] = sig.raw.hex()
    args.json.write_text(json.dumps(data, indent=2) + "\n")
    print(f"signed {args.json} (pubkey {EXPECTED_PUB[:16]}…, sig {data['signature'][:16]}…)", file=sys.stderr)
    # best-effort wipe
    sec = b"\0" * 32
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
