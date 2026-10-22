# DRAFT — PDC 3.0.0.12 (superseded — do not publish)

**Status:** **Superseded by [3.1.0](RELEASE_NOTES_3.1.0.md)** (same build **12**). Do **not** create tag `v3.0.0.12`.

**Planned tag:** `v3.0.0.12`  
**Base branch for cut:** `dev` → `master` (after soak)  
**Previous release:** `v3.0.0` / build 11 (`aa3985a`)

## Compatibility with `v3.0.0` (build 11)

- **Wire / P2P:** HF8 min build stays **11** (`ZANO_HARDFORK_08_MIN_BUILD_VER`). Build **12** peers remain compatible with release **11** nodes (no handshake raise).
- **RPC:** `getinfo` tip hash/timestamp is **additive** — clients that ignored empty `last_block_hash` keep working; flag-gated reward fields unchanged.
- **No consensus / hard-fork height change** in this draft.

## Highlights

- **`getinfo` always returns tip `last_block_hash` / `last_block_timestamp`** without requiring `TOTAL_COINS` / `POS_DIFFICULTY` flags (cheap tip read). Required by oracle sidechain CA attestation.
- **Ops:** `ops/pdcx-oracle-fee/` — PDCx fee-token descriptor + rollback-friendly `swap-pdcd-rollback.sh` (binary swap / restore). Mainnet PDCx fee asset already deployed (usage reversible via oracle fee amount / flags; see ops README).

## Not in this binary release

- Creating/publishing a GitHub Release or CI `release:published` artifact upload.
- Whitelist entry for a separate **wrapped PDCX** (1:1 PDC) product — that remains a distinct ticker/flow from the oracle **PDCx fee** CA.

## Local test plan (before tag)

1. Build CLI from this tree (`cmake` + `make daemon` / platform CI `workflow_dispatch` without release event).
2. `pdcd --version` → `3.0.0.12[...]`.
3. Against a synced node (or local):
   ```bash
   curl -s http://127.0.0.1:19211/getinfo | jq '{height,last_block_hash,last_block_timestamp}'
   ```
   Expect non-empty `last_block_hash` with **empty flags** / default `/getinfo`.
4. JSON-RPC:
   ```bash
   curl -s -X POST http://127.0.0.1:19211/json_rpc \
     -H 'Content-Type: application/json' \
     -d '{"jsonrpc":"2.0","id":0,"method":"getinfo","params":{}}' \
     | jq '.result|{height,last_block_hash}'
   ```
5. Optional: run `ops/pdcx-oracle-fee/local-smoke-tip-hash.sh` (see script).
6. Rollback drill on a non-prod binary path: install candidate → `swap-pdcd-rollback.sh rollback`.

## Publish checklist (later — operator only)

- [ ] Local / staging soak signed off  
- [ ] Merge `dev` → `master`  
- [ ] `git tag -s v3.0.0.12` + `gh release create` (triggers artifact upload)  
- [ ] Host binary swap with `pdcd.prev` retained  
