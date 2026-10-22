# Shadow `pdcd` on mainnet host (zero impact on live)

## Verdict

**Yes — safe to test 3.0.0.12 on the same mainnet host without touching the live stack**, if and only if:

| Rule | Live (must not change) | Shadow (test only) |
|---|---|---|
| Binary | `/home/ubuntu/pdc/pdcd` | separate path e.g. `/home/ubuntu/pdc-shadow/pdcd` |
| systemd | `pdcd.service` stays running | **no** unit replace; optional throwaway `pdcd-shadow.service` or manual process |
| data-dir | `/home/ubuntu/.PDC` | **new** dir e.g. `/home/ubuntu/.PDC-shadow-12` (never share LMDB) |
| P2P | `0.0.0.0:19121` | `127.0.0.1:19131` (loopback only) |
| RPC | `127.0.0.1:19211` | `127.0.0.1:19221` |
| Consumers | Trade / oracle / wallets / pool keep `19211` | smoke scripts only → `19221` |

Do **not** run `swap-pdcd-rollback.sh install` for this soak — that swaps the live binary.

## Why this does not break production

- Tip-hash change is **RPC additive** (no consensus / HF height change).
- Shadow process uses other ports + other data-dir → no LMDB lock fight with live.
- Oracle / Trade / mint wallets stay on `19211`.
- Shadow P2P bound to loopback + `--add-peer=127.0.0.1:19121` syncs from the live node without opening a second public P2P listener.

## Procedure

See `run-shadow-pdcd.sh`.

## Cleanup

```bash
./run-shadow-pdcd.sh stop
# optional: rm -rf /home/ubuntu/.PDC-shadow-12 /home/ubuntu/pdc-shadow
```

Live `pdcd.service` is never stopped by these scripts.
