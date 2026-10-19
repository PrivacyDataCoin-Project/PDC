# HF8 + PoS sequence limit rollout

## Timeline (mainnet)

| Height | Event |
|--------|--------|
| now (~30k) | Deploy **PDC 3.0.0.11** with PoS asymmetry fix + HF8 schedule |
| after deploy | Broadcast maintainers alert (urgent/critical for older builds) |
| **50000** | **HF8** activates; peers with `build < 11` are dropped |
| **57000+** | Strict PoS sequence limit (max 21 consecutive PoS) on main and alt chain |

Testnet: HF8 after **15000**, PoS limit after **18000**.

## What changed in 3.0.0.11

- PoS sequence-limit asymmetry fix (alt chain matches main chain; Zano `e00bbabc`).
- HF8 scheduled before the PoS height gate so operators must upgrade first.
- `BLOCKCHAIN_HEIGHT_FOR_POS_STRICT_SEQUENCE_LIMITATION` stays **57000** (unchanged).
- No emergency blockchain truncate on startup.

## Maintainers notification (requires private key)

Public key (mainnet): `P2P_MAINTAINERS_PUB_KEY` / `P2P_MAINTAINERS_MAINNET` in `src/currency_core/currency_config.h`.

Example JSON: [maintainers_info_hf8.json](maintainers_info_hf8.json) (`3.0.0` build `11`).

```bash
./connectivity_tool \
  --ip=127.0.0.1 \
  --rpc-port=19211 \
  --update-maintainers-info=docs/maintainers_info_hf8.json \
  --private-key=<HEX_SECRET_MATCHING_P2P_MAINTAINERS_PUB_KEY>
```

- Signs payload against `P2P_MAINTAINERS_PUB_KEY`, then `POST /set_maintainers_info.bin`.
- Prefer localhost RPC (endpoint is denied on public nginx allowlist).
- Never commit the private key.

Alert modes: `1` calm, `2` urgent, `3` critical. Conditions apply when local `PROJECT_VERSION_BUILD_NO < if_build_less_then`.

## Operator checklist

1. Upgrade seed / pool / explorer daemons to **3.0.0.11** before height 50000.
2. Push maintainers info from an admin daemon after the release is live.
3. Confirm `pdcd --version` shows `3.0.0.11[...]`.
4. After height 50000, verify old peers are disconnected.
5. After height 57000, confirm PoS mining pauses when sequence factor exceeds 20 until a PoW block arrives.
