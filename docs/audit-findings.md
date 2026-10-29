# PDC core audit

Date: 2026-10-29. Scope: `PrivacyDataCoin-Project/PDC` core (this repository). Findings below were read in the tree. Items that were not reproduced in code are not listed.

## Architecture

C++17 node (CMake `CMAKE_CXX_STANDARD` 17; Unix flags also pass `-std=c++14`). Release CI pins GCC 11 / MSVC 2022, Boost 1.84, OpenSSL 3.5.x, Qt 6.8.3 for the GUI only. `CMakeLists.txt` project version is `3.1.0`.

| Target | Role |
|---|---|
| `pdcd` (`daemon`) | Node: `currency_core` + P2P + `core_rpc_server` + optional stratum |
| `simplewallet` | CLI wallet |
| `connectivity_tool` | Chain / connectivity helper |
| `Pdc` | Qt GUI (`BUILD_GUI`) |
| `unit_tests`, `coretests`, `functional_tests` | `BUILD_TESTS=ON` |

Libraries under `src/`: `common`, `crypto` (Curve25519, CLSAG, range proofs, Zarcanum, ChaCha8), `currency_core` (chain, mempool, PoW RandomARQ, PoS, assets, aliases), `wallet`, `rpc`, `stratum`. Vendored pieces live in `contrib/` (epee HTTP/P2P, LMDB/MDBX, RandomARQ, ethash, zlib). Submodules: RandomARQ, miniupnp, tor-connect, jwt-cpp.

There is no Solidity/EVM execution on this L1. `oracle_update` is not a consensus transaction in `currency_core`. Public daemon RPC policy is operational (nginx + `ops/pdcd-public-rpc/json_rpc_filter.py`): `pdcd` is intended to bind `127.0.0.1` and expose a read/sync allowlist, including `get_alt_blocks_details`.

### Critical paths

Block on the main chain: `blockchain_storage::add_new_block` → `prevalidate_block` → `handle_block_to_main_chain` (timestamp, checkpoints, miner tx, PoW `check_hash_64` or PoS `validate_pos_block`, per-tx semantic + balance + input signatures outside the checkpoint zone, miner reward).

Mempool: `tx_memory_pool::add_tx` → hardfork terms → `validate_tx_semantic` → `check_tx_inputs` → post-HF4 `check_tx_balance` and `validate_asset_operation`.

Wallet send: `transfer` / `prepare_transaction` → `construct_tx` → `finalize_transaction` → relay. Scan: `process_new_transaction` → `lookup_acc_outs`. Wallet file: ChaCha8 over keys (`store_keys`) and over the body (`store`).

Core RPC: `core_rpc_server`, default bind `127.0.0.1`, no credential check. GUI companion mode (`wallets_manager::setup_wallet_rpc`) chains wallet JSON-RPC onto that port and sets a JWT secret on `wallet_rpc_server`.

## Fixes in this change

| Severity | Issue | Where | Change |
|---|---|---|---|
| High | `get_alt_blocks_details` uint64 wrap, then `vector::erase` past the end | `src/currency_core/blockchain_storage.cpp` `get_alt_blocks_rpc_details` | `clamp_rpc_range_count` saturates `count` to `size - start_offset` |
| High | Companion JWT skipped for `mw_get_wallets` / `mw_select_wallet` | `src/wallet/wallets_manager.cpp` | Those two methods use `wallet_rpc_server::auth_http_request` when a JWT secret is set. Other methods stay on the existing chain and are checked once |
| High | Watch-only file encrypts the body with a different password than the keys | `src/wallet/wallet2.cpp` `store` | Body uses the `password` argument, same as `store_keys` and `load` |
| Medium | HTTP `Content-Length` has no upper bound | `contrib/epee/include/net/http_protocol_handler.inl` | Reject digit strings longer than 20, cast failures, and bodies above 64 MiB. This is the request body only |
| Low | `store_keys` ignores `dump_binary` failure | `src/wallet/wallet2.cpp` | Return that result |
| Low | `start_mining` thread count is unbounded | `src/currency_core/miner.cpp` `miner::start` | Cap at 256 threads |

`clamp_rpc_range_count` matches the old clamp for every non-wrapping input: `start + count >= size` becomes `count = size - start`, including equality.

## Findings not changed

### High — cold-sign / watch-only submit does not handle Zarcanum outputs

`wallet2::sign_transfer` (`src/wallet/wallet2.cpp`, `tx_out_zarcanum` branch is empty) never records change key images for HF4 outputs. `wallet2::submit_transfer` requires `txin_to_key` and `tx_out_bare`. A post-HF4 cold-sign flow throws or omits key images. Filling this in means deriving key images. That was not done here: a wrong derivation would mark the wrong outputs spent. No consensus change.

### Medium — full core RPC is unauthenticated by design

`reset_transaction_pool`, `remove_tx_from_pool`, `/force_relay` (relay without `handle_incoming_tx`), and `/start_mining` have no shared secret. Defaults and `docs/PDCD_PUBLIC_RPC_POLICY.md` keep that surface on `127.0.0.1`. The public filter already denies the destructive methods. `/force_relay` is a separate endpoint from `/sendrawtransaction` and is named as a forced relay. Not removed. Residual risk: binding `pdcd` on a public address without the filter.

`get_blocks_details` / `get_main_blocks_rpc_details` has no count cap. The loop is bounded by chain height (`i < m_db_blocks.size()`), so the uint64 add does not walk off the container. A large `count` still makes one heavy response. Left unchanged so explorers that page by height keep working. The public filter allows this method.

`check_keyimages.bin` accepts an unbounded image list. Same class of RPC cost. Not capped in this change; no existing limit in the handler to restore.

### Low — `reset_wallet_password` does not write the file itself

`wallet2::reset_password` only assigns `m_password`. `wallets_manager::reset_wallet_password` returns OK without `store()`. A normal shutdown calls `store()` (`wallets_manager::main_worker`), which then encrypts with the new password. A crash before that store leaves the old password on disk. Not changed: forcing `store()` from the setter can fail for unrelated I/O reasons and is not what the current callers do.

### Low — HF5 hidden-supply emit/burn: validate vs apply

`validate_asset_operation_hf5` does not reject emit/burn when `last_adb.hidden_supply` is set. `put_asset_info` returns false for that case (`not supported atm`). `validate_asset_operation_amount_commitment` returns false when the operation's own descriptor has `hidden_supply`, and `validate_ado_initial` / `validate_ado_update_allowed` do not provide a path that sets `hidden_supply` to true (the field is default false; production assignments found are `= false`). The mismatch is real in the two functions and not reachable while no hidden-supply asset can be registered. Consensus validation was not tightened.

### Checked and not reported as defects

- Pre-HF4 bare in/out overflow checks in `validate_tx_semantic`.
- Post-HF4 `check_tx_balance` and range/surjection proofs on the non-checkpoint block path.
- PoW 32-bit nonce gate and `check_hash_64`.
- PoS `validate_pos_block` / Zarcanum proof on the main chain.
- `load_keys` checks spend/view secrets against the loaded public keys.
- Direct `wallet_rpc_server::handle_http_request` does enforce JWT, body hash, and single-use salt when the secret is set. The hole was only the `wallets_manager` map in front of that chain.
- `mw_get_wallets` copies `wallet_info` (including `view_sec_key` from `get_wallet_info`). It does not copy `open_wallet_response::pass`.
- No `system()` under `src/`.

## Verification

Host used for this pass: Ubuntu, g++ 13.3, system Boost 1.83, OpenSSL 3.0.13. Not the release-CI pin (Boost 1.84 / OpenSSL 3.5.8). Tor disabled for this build (`-DDISABLE_TOR=ON`). GUI not built.

```
export PDC_USE_SYSTEM_BOOST=1 PDC_DISABLE_LTO=1
cmake -S . -B build -DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++ \
  -DCMAKE_BUILD_TYPE=Release -DSTATIC=OFF -DBUILD_GUI=OFF -DBUILD_TESTS=ON \
  -DDISABLE_TOR=ON
cmake --build build --parallel --target unit_tests
./build/tests/unit_tests --gtest_filter='clamp_rpc_range_count.*'
```

Submodules required for configure: `contrib/randomx`, `contrib/jwt-cpp`, `contrib/miniupnp` (`git submodule update --init`). `contrib/tor-connect` is required unless `-DDISABLE_TOR=ON`.

Run on this tree (g++ 13.3, Boost 1.83, OpenSSL 3.0.13, `-DDISABLE_TOR=ON`, `-DBUILD_GUI=OFF`, `-DSTATIC=OFF`):

- `cmake --build build --parallel 2 --target unit_tests` completed (exit 0).
- `./build/tests/unit_tests`: 538 tests, 537 passed, 1 failed.
- `clamp_rpc_range_count.*`: 4/4 passed, including `offset=1`, `count=2^64-1`.

The failing test is `auditable_addresses.basic` in `tests/unit_tests/base58.cpp`. The vectors are Zano-style strings (`Zx…`, `iZ…`, `aZx…`). This build's prefixes are PDC (`CURRENCY_PUBLIC_ADDRESS_BASE58_PREFIX` `0x1989`, addresses start with `Px`). The failure is prefix rejection and is not touched by the fixes above.

Release-CI pins (Boost 1.84, OpenSSL 3.5.8, static link, tor enabled) were not reproduced here.
