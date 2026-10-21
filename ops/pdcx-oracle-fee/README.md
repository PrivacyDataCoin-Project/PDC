# PDCx (oracle attestation fee) — mainnet, rollback-friendly

## Purpose

`PDCx` is the **payment asset** for sidechain CA→EVM attestation fees on the **PDC** network (not native wei / ETH).

## Deploy (mainnet)

1. Review `asset-descriptor.pdcx.json` (initial supply = 10 000 PDCx @ 8 decimals).
2. From a maintainer wallet (simplewallet):

```bash
deploy_new_asset /path/to/asset-descriptor.pdcx.json
```

3. Record `Asset ID` + tx id in `DEPLOYED.md` (gitignored secrets stay out of git).
4. Point oracle env:

```bash
ATTESTATION_FEE_ASSET_ID=<64hex>
ATTESTATION_FEE_AMOUNT=<atomic units>
# ATTESTATION_FEE_WEI is ignored / must stay 0
```

5. EVM: deploy `PdcSidechainAttestationTokenFee.sol` with ERC-20 that represents PDCx (or a bridge wrap). Keep `paused=true` until soak completes; unpause to enable fees. **Rollback:** `pause()` or `setAttestationFee(0)` + oracle `SIDECHAIN_ATTESTATION_ENABLED=0`.

## Rollback (pdcd binary)

```bash
# After binary swap:
sudo systemctl stop pdcd
cp -a /home/ubuntu/pdc/pdcd.prev /home/ubuntu/pdc/pdcd
sudo systemctl start pdcd
```

Asset `PDCx` itself remains on-chain if already deployed; fee *usage* is disabled via oracle/EVM pause — that is the reversible control surface.
