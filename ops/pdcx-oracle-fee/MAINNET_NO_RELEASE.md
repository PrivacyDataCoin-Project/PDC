# Mainnet tip-hash / PDCx fee — prepare **without** GitHub release

Do **not** publish `3.0.0.12` GitHub Release / tag until shadow soak is green. Use `dev` commit or a local/CI binary.

## Safe path (recommended)

1. Build or copy `pdcd` **into shadow path only** (never overwrite `/home/ubuntu/pdc/pdcd` yet):
   ```bash
   ./ops/pdcx-oracle-fee/install-shadow-binary.sh /path/to/built/pdcd
   ./ops/pdcx-oracle-fee/run-shadow-pdcd.sh preflight
   ./ops/pdcx-oracle-fee/run-shadow-pdcd.sh start
   RPC_URL=http://127.0.0.1:19221 ./ops/pdcx-oracle-fee/local-smoke-tip-hash.sh
   ./ops/pdcx-oracle-fee/run-shadow-pdcd.sh status
   ```
2. Live `pdcd.service` stays on current binary; consumers stay on `19211`.
3. Only after soak: optional live swap via `swap-pdcd-rollback.sh` (separate decision — still no GitHub release required).

## Oracle fee ENV (host)

Already documented in [README.md](./README.md). Merge without release:

```bash
ATTESTATION_FEE_ASSET_ID=5292b7984565b21d47e456c381973b700738b9819f9e96b841f68cfa116774bd
ATTESTATION_FEE_AMOUNT=0
SIDECHAIN_ATTESTATION_ENABLED=0
```

## Cross-repo

| Component | Pack |
|---|---|
| Oracle | `oracle/deploy/mainnet-no-release/` |
| Trade shadow | `pdc_trade_backend/deploy/shadow-pdcx/MAINNET_NO_RELEASE.md` |
| This | shadow `pdcd` only |

## Explicitly forbidden in this prepare step

- `gh release create` / git tag for 3.0.0.12
- `swap-pdcd-rollback.sh install` on live without separate approval
- Sharing LMDB / ports with live
