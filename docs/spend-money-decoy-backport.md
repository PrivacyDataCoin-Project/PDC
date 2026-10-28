# Spend-money decoy selection backport (AMOUNTS4)

## Why

Upstream Zano spend fixes (PRs #765, #771, #773) depend on the AMOUNTS4 decoy
RPC and bare-output-by-global-index selection. PDC previously only had AMOUNTS3
(`getrandom_outs3`), which returns flat offset pools and cannot express the
height-based / strategy-based / bare-index batch model those fixes require.

Porting only the three tiny wallet diffs is not enough: the wallet now builds
`COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS4` requests (one batch per input,
optional bare candidates that include the real global index, then shuffled).

## Upstream PRs

| PR | Role |
|----|------|
| #765 | Bare decoy candidates by global index; include real output in the pool |
| #771 | Shuffle bare `global_offsets` so the real index is not at a fixed position |
| #773 | One AMOUNTS4 batch per input (do not merge same-amount inputs) |
| #757 | Transfer callback / `long_refresh_in_progress` on `wallet_id_adapter` (ported alongside) |

AMOUNTS4 also adds look-up strategies (`LOOK_UP_STRATEGY_REGULAR_TX`,
`LOOK_UP_STRATEGY_POS_COINBASE`, `LOOK_UP_STRATEGY_ANY`) used for regular spends
and Zarcanum PoS coinbase decoys.

## What landed in PDC

- RPC: `COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS4`, `/getrandom_outs4.bin`, JSON `getrandom_outs4`
- Daemon: `blockchain_storage::get_random_outs_for_amounts4`, `get_bare_outs_by_indices`, block strategy helpers
- Wallet proxies: `call_COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS4` (keeps PDC `core_rpc_proxy.h` naming)
- Wallet: `prepare_tx_sources` uses AMOUNTS4; bare planning helpers; PoS ZC path via AMOUNTS4
- Snapshot: `bare_outputs_snapshot.{h,cpp}` — **empty by default** (safe). Do not reuse Zano counts.
- Daemon command: `export_bare_outputs <file.cpp>` regenerates PDC-chain counts after sync past HF4
- Shared planner: `src/wallet/bare_decoy_batch_planner.h` (unit-tested; wallet2 bare append uses it)
- Adapter: PR #757 callback / long-refresh on `i_wallet_to_i_backend_adapter`

PDC-only APIs (`burn_native`, assets whitelist signing blob, etc.) are unchanged.

### Bare outputs snapshot (critical)

Wrong-chain bucket sizes bias bare decoy sampling (privacy regression). PDC ships an empty
map so the wallet samples the full index domain and the daemon applies the real
`outputs_count`. After mainnet sync past HF4:

```text
export_bare_outputs src/wallet/bare_outputs_snapshot.cpp
```

Verify the generated `Genesis:` line matches `CURRENCY_GENESIS_HASH` before committing.

## How to test

Unit (no daemon):

```bash
# from a configured build directory
ctest -R bare_decoy_batch_planner --output-on-failure
# or
./unit_tests --gtest_filter='bare_decoy_batch_planner.*'
```

Invariants covered:

1. Bare append includes `real_gindex` and requests `decoy_count` extra candidates.
2. After shuffle (fixed RNG seed), real index is still present and order can change.
3. N same-amount inputs produce N separate batches.

Integration / coretests: Zano `hardfork_4_bare_decoy_*` cases exercise live daemon
AMOUNTS4 behavior; porting those into PDC `tests/core_tests/hard_fork_4.*` is a
follow-up once the AMOUNTS4 daemon path is exercised in this tree.

## Apply notes

- Prefer keeping `core_rpc_proxy.h` (do not rename to Zano's `i_core_rpc_proxy.h`).
- Wallet CMake uses `GLOB_RECURSE` over `src/wallet/*`; new `.cpp` files are picked up automatically.
- Unit tests under `tests/unit_tests/` are also globbed by `tests/CMakeLists.txt`.
