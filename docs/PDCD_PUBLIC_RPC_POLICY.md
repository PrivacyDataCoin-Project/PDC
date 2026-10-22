# Polityka publicznego RPC `pdcd` (R-01)

**Data:** 2026-10-18  
**Host:** `169.58.142.131`  
**Cel:** iOS/remote wallets: sync + transfer; pool + portfele poola: pełny dostęp lokalny; Internet: bez otwartego kopania.

## Konsumenci (macierz)

| Konsument | Jak łączy się dziś | Po R-01 | Potrzebuje |
|-----------|--------------------|---------|------------|
| **iOS / remote wallets** | `169.58.142.131:19211` | nginx allowlist na publicznym IP:19211 | sync (`getinfo`, `getblocks.bin`, `getrandom_outs*`, …) + **transfer** (`/sendrawtransaction`) |
| **Mining pool (`pdcpool`)** | `daemon.host=127.0.0.1` `port=19211` | **bez zmian** → pełne RPC localhost | `getblocktemplate`, `submitblock*` |
| **Portfel poola PDC** | `simplewallet --daemon-address=127.0.0.1:19211` + wallet-rpc `:19212` | **bez zmian** | sync + transfer + payout via wallet-rpc |
| **Trade / Explorer** | `127.0.0.1:19211` | **bez zmian** | read / asset / alias JSON-RPC |
| **Losowy Internet** | wcześniej pełne RPC | **deny** mining + destructive | — |

## Architektura (wybrana: split bind, ten sam port)

```
Internet / iOS ──► 169.58.142.131:19211 (nginx allowlist)
                         │
                         ├─ HTTP allow: getinfo, getheight, gettransactions,
                         │              sendrawtransaction, *.bin sync
                         └─ /json_rpc ──► 127.0.0.1:19215 (filtr metod)
                                              │
pdcpool / poolwallet / Trade / Explorer       ▼
         ──► 127.0.0.1:19211 (pdcd FULL RPC)
```

`pdcd` bindowany wyłącznie na `127.0.0.1:19211`. Nginx nasłuchuje **tylko** na adresie publicznym `:19211` (nie na `0.0.0.0`), więc lokalne usługi nie przechodzą przez allowlistę.

## Allowlist publiczna (Internet / iOS)

### HTTP (sync + transfer)

- `/getinfo`, `/getheight`, `/gettransactions`
- **`/sendrawtransaction`** — broadcast transferów (wallet2 / iOS FFI); **wymagane**
- Binary: `/getblocks.bin`, `/get_o_indexes.bin`, `/getrandom_outs.bin`, `/getrandom_outs1.bin`, `/getrandom_outs3.bin`, `/get_tx_pool.bin`, `/check_keyimages.bin`

### JSON-RPC (tylko odczyt / sync helper)

`getinfo`, `getblockcount`, block headers, alias/asset/pool **read**, `getrandom_outs*`, `get_blocks_details`, `get_tx_details`, `search_by_id`, `validate_signature`, …

Źródła: `src/wallet/core_default_rpc_proxy.cpp`, `pdc-ios-wallet` (remote-node + FFI), `react-native-zano` `sendrawtransaction`.

## Denylist publiczna (Internet)

| URI / method | Dlaczego |
|--------------|----------|
| `/start_mining`, `/stop_mining` | hijack CPU / coinbase |
| `getblocktemplate`, `submitblock`, `submitblock2` | mining — **tylko localhost (pool)** |
| `/force_relay` | admin relay |
| `reset_transaction_pool`, `remove_tx_from_pool` | destructive |
| `/set_maintainers_info.bin`, `/get_pos_details.bin` | admin / PoS mining details |

## Lokalnie (pool + portfele poola) — FULL

Bez filtrów: `127.0.0.1:19211` (daemon) oraz `127.0.0.1:19212` (pool wallet-rpc).  
**Nie** zmieniać `pdcpool/config.json` ani cmdline poolwallet przy tym wdrożeniu.

## Ops / pliki

`ops/pdcd-public-rpc/` — nginx, filtr Python, snippet `pdcd.service`.

| Unit / path | Role |
|-------------|------|
| `pdcd-json-rpc-filter.service` | Allowlist on `127.0.0.1:19215` (must be **active**) |
| nginx site `pdcd-public-rpc` | Public `:19211` → HTTP allowlist + `/json_rpc` → filter |
| `pdcd` | Bind **only** `127.0.0.1:19211` (full RPC for pool/Trade) |

### Status (2026-10-22)

- HTTP allowlist (getinfo / sendrawtransaction / mining deny) — **live**
- JSON-RPC filter must stay running; if inactive, public `/json_rpc` returns **502**
- Restart: `sudo systemctl enable --now pdcd-json-rpc-filter && sudo systemctl reload nginx`

### Not a public EVM L2

Oracle attestation may use a **private Clique** EVM (localhost geth, e.g. `127.0.0.1:18545`, chainId `97421001`) for fee/tip soak. That surface is **not** exposed on the public IP. Public remote wallets use **pdcd RPC allowlist** above — not a public L2 RPC.

### Weryfikacja

```bash
# Public: mining DENY (403, nie "Failed, wrong address")
curl -sS -o /tmp/m.txt -w "%{http_code}\n" -X POST http://169.58.142.131:19211/start_mining \
  -H 'Content-Type: application/json' -d '{"miner_address":"INVALID","threads_count":1}'

# Public: sync OK
curl -sS -X POST http://169.58.142.131:19211/getinfo -H 'Content-Type: application/json' -d '{}'

# Public: JSON-RPC read OK (filter up); mining method → 403
curl -sS -X POST http://169.58.142.131:19211/json_rpc -H 'Content-Type: application/json' \
  -d '{"jsonrpc":"2.0","id":1,"method":"getinfo","params":{}}'
curl -sS -o /tmp/j.txt -w "%{http_code}\n" -X POST http://169.58.142.131:19211/json_rpc \
  -H 'Content-Type: application/json' \
  -d '{"jsonrpc":"2.0","id":1,"method":"getblocktemplate","params":{}}'

# Public: transfer path allowed (invalid blob → daemon error, nie 403)
curl -sS -o /tmp/tx.txt -w "%{http_code}\n" -X POST http://169.58.142.131:19211/sendrawtransaction \
  -H 'Content-Type: application/json' -d '{"tx_as_hex":"00"}'

# Local pool: template still works
curl -sS -X POST http://127.0.0.1:19211/json_rpc -H 'Content-Type: application/json' \
  -d '{"jsonrpc":"2.0","id":"0","method":"getblocktemplate","params":{"wallet_address":"INVALID","reserve_size":0}}'
```

Cross-ref: `REMEDIATION_PROCESS.md` (R-01 / G7), `AUDIT_MINING_API.md`.
