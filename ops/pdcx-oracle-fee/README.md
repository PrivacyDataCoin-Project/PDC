# PDCx (oracle attestation fee) — mainnet, rollback-friendly

## Purpose

`PDCx` is the **payment asset** for sidechain CA→EVM attestation fees on the **PDC** network (not native wei / ETH).

## Mainnet (deployed)

| Field | Value |
|---|---|
| ticker | `PDCx` |
| asset_id | `5292b7984565b21d47e456c381973b700738b9819f9e96b841f68cfa116774bd` |
| tx_id | `977bde4b38d065702e8483523ea5c9300b147bdb00c58d4f4f510f968e4601ac` |
| decimals | 8 |
| initial supply | 10 000 PDCx |

Oracle host env (reversible):

```bash
ATTESTATION_FEE_ASSET_ID=5292b7984565b21d47e456c381973b700738b9819f9e96b841f68cfa116774bd
ATTESTATION_FEE_AMOUNT=1000000   # 0.01 PDCx; set 0 to stop charging
ATTESTATION_FEE_WEI=0            # ignored / retired
```

EVM: deploy `PdcSidechainAttestationTokenFee.sol` **paused** with ERC-20 wrap of PDCx when ready. **Rollback:** `pause()` or fee=0 + `SIDECHAIN_ATTESTATION_ENABLED=0`.

## Rollback (pdcd binary)

```bash
# After binary swap:
sudo systemctl stop pdcd
cp -a /home/ubuntu/pdc/pdcd.prev /home/ubuntu/pdc/pdcd
sudo systemctl start pdcd
```

Asset `PDCx` itself remains on-chain if already deployed; fee *usage* is disabled via oracle/EVM pause — that is the reversible control surface.

## Mainnet prepare without GitHub release

See [MAINNET_NO_RELEASE.md](./MAINNET_NO_RELEASE.md) and [shadow-pdcd-mainnet.md](./shadow-pdcd-mainnet.md).
Prefer shadow install:

```bash
./ops/pdcx-oracle-fee/install-shadow-binary.sh /path/to/built/pdcd
./ops/pdcx-oracle-fee/run-shadow-pdcd.sh preflight
```
