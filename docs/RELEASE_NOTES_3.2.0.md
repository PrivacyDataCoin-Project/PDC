# DRAFT — PDC 3.2.0 (build 13)

**Status:** version bump prepared. **Do not** create git tag `v3.2.0` or GitHub Release until soak and operator sign-off.

**Planned tag:** `v3.2.0`  
**Previous release:** `v3.1.0` / build 12  
**GUI pin:** `pdc_ui` @ `27489f3` (latest `origin/master` at prepare time)

## Compatibility

- **Wire / P2P:** HF8 min build stays **11**. Build **13** remains compatible with release **11** and **12** peers.
- **No new hard-fork height** in this draft.

## Highlights (planned)

### Wallet / spend path

- Backport Zano **AMOUNTS4** decoy selection stack (spend-money fixes; see PR #8).
- PDC export path for bare outputs (no Zano `bare_outputs_snapshot`).

### Carry-forward from 3.1.0

- GUI: PDCX / ARQX logos, Trade-backed USD marks via explorer `pdc_market`.
- `getinfo` tip `last_block_hash` / `last_block_timestamp`.
- Console: `status`, `version`, `print_net_stats`.

## Local check (before tag)

1. `pdcd --version` → `3.2.0.13[...]`
2. Transfer / decoy selection smoke on mainnet or testnet wallet.
3. GUI still embeds `pdc_ui` @ `27489f3`.

## Publish checklist (later — operator only)

- [ ] Soak signed off  
- [ ] Merge to `master`  
- [ ] `git tag -s v3.2.0` + `gh release create`  
- [ ] CI `release:published` artifacts  
