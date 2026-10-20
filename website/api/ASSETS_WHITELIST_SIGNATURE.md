# assets_whitelist.json — signature (R-02a)

**Updated (2026-10-20):** whitelist = **BTCX** + **ARQX** (same mint `owner`
`50bb21f31675bfa40947586a0d01f13b8510c7143e6f8770e951c53bc8587abb`);
`current_supply` from live `get_asset_info` (refresh when peg changes);
`signature` is a real Cryptonote signature under
`WALLET_ASSETS_WHITELIST_VALIDATION_PUBLIC_KEY`
(`1bfd31dc840c0f205d9b72df18d298cbdef716e4389684c7da19f3db2a77c65f`).

## Algorithm (must match `wallet2::load_whitelisted_tokens`)

For each asset in order, append UTF-8 lines (each terminated by `\n`):

1. `asset_id` — 64 hex lowercase  
2. `total_max_supply` — decimal  
3. `current_supply` — decimal  
4. `decimal_point` — decimal  
5. `ticker`  
6. `full_name`  
7. `meta_info` (empty string if absent)  
8. `owner` — 64 hex lowercase  
9. `hidden_supply` — `1` or `0`

Then:

```text
h = cn_fast_hash(blob)
crypto::generate_signature(h, validation_pub, validation_sec, signature)
```

`signature` in JSON is 128 hex chars (64-byte Cryptonote `c||r`). All-zero signatures are rejected by the wallet.

## Re-sign

```bash
# Requires utils/pdc_authority_keys.SECRET.txt (gitignored) and a PDC binary
# that exports cn_fast_hash / crypto_ops::generate_signature (e.g. connectivity_tool).
python3 utils/sign_assets_whitelist.py \
  --json website/api/assets_whitelist.json \
  --connectivity-tool /path/to/connectivity_tool
```

Do **not** invent a new keypair — it would not match `currency_config.h`.
