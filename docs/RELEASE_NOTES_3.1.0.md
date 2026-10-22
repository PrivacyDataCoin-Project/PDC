# PDC 3.1.0 (build 12)

**Status:** version bump prepared on `dev`. Publish tag / GitHub Release only after soak and operator sign-off.

**Planned tag:** `v3.1.0`  
**Base:** `dev` → `master` (after soak)  
**Previous release:** `v3.0.0` / build 11  
**Supersedes draft:** `3.0.0.12` (never tagged; build number **12** reused for this release)

## Compatibility

- **Wire / P2P:** HF8 min build stays **11**. Build **12** remains compatible with release **11** peers.
- **No new hard-fork height** in this release.
- **RPC:** `getinfo` tip `last_block_hash` / `last_block_timestamp` always filled (additive).

## Highlights

### Wallet GUI (`pdc_ui` @ `27489f3`)

- **PDCX** wrap ticker labeling + same readable padlock logos as PDC.
- **ARQX** Arqma icon forced over `custom_token`.
- Sidebar portfolio USD from Trade marks via explorer `pdc_market` (CORS-safe).

### Daemon / CLI

- Console: `status`, `version`, `print_net_stats` (in-process, no network RPC).
- `print_market` soft-fails with enable hint when `--enable-offers-service` is off.
- `getinfo` always returns tip hash/timestamp (oracle / sidechain attestation).
- CLI-scoped macOS CI (`build=cli-arm64` / `cli` / `cli-x64`).

### Ops (docs / scripts; not consensus)

- PDCx oracle fee asset notes + mainnet shadow `pdcd` path (no live binary swap required for soak).
- Signed assets whitelist includes **ARQX**.

## GUI pin

Submodule `src/gui/qt-daemon/layout` → **`27489f3`** (`pdc_ui` `origin/master` at prepare time).

## Local check

1. `pdcd --version` → `3.1.0.12[...]`
2. GUI build embeds layout at `27489f3` (PDC / PDCX / ARQX icons).
3. `getinfo` tip hash non-empty without special flags.
4. Optional: `ops/pdcx-oracle-fee/local-smoke-tip-hash.sh`

## Publish checklist (operator)

- [ ] Soak signed off  
- [ ] Merge `dev` → `master`  
- [ ] `git tag -s v3.1.0` + `gh release create` (CI artifacts)  
- [ ] Maintainers alert already live for builds `< 11`; no change required for 3.1.0  
