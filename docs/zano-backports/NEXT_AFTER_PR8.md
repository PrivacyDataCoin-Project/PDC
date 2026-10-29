# Zano backports after PDC PR #8 (AMOUNTS4)

Base for follow-ups: merge or stack on
[`cursor/spend-money-decoy-fix-6c11`](https://github.com/PrivacyDataCoin-Project/PDC/pull/8)
(Zano spend-money #765 / #771 / #773 + #757).

Upstream tip compared: `hyle-team/zano` master @ `38aaa9d` (2026-10-28).

## Queue

| Order | Zano | Title | PDC status | Proposed PDC PR |
|------:|------|-------|------------|-----------------|
| — | #765 #771 #773 #757 | AMOUNTS4 / spend-money + transfer adapter | **In PR #8** | PR #8 |
| — | #767 | AppImage NSS/NSPR bundle | **Already in** `build-linux.yml` | — |
| — | #769 | `export_history` → `export_recent_transfers` | **Already in** `simplewallet.cpp` | — |
| — | #770 | Sync reconnect after EOF (`m_connected=false`, reset `try_count`) | **Already in** epee + `wallet2::refresh` | — |
| **1** | **#772** | Legacy full block sync via `m_return_legacy_full` | **Next** | this branch / draft PR |
| 2 | #750 remainder | Old-client compact cutoff (`m_return_compact` must be true or GENESIS_MISMATCH) | Optional; Zano has it, PDC RPC does not | separate if desired |
| 3 | coretests | `hard_fork_4` / `get_random_outs` bare-decoy cases | Not ported (PDC has planner unit tests only) | after #8 + live daemon exercise |

## PR #772 — what to change in PDC

Upstream files: `core_rpc_server_commands_defs.h`, `core_rpc_server.cpp`,
`core_default_rpc_proxy.cpp`, `wallet2.cpp`, `wallet2.h`
(+ Zano-only `wallet_compact_sync` tests — **skip** unless/until compact sync
coretests are ported).

Semantics:

1. Keep `m_return_compact` as the **compatibility / old-client** flag (Zano
   emergency cutoff). PDC does not yet enforce that cutoff on RPC; still send
   `m_return_compact = true` from the wallet so a future cutoff stays safe.
2. Add `m_return_legacy_full` (default `false`) to select full vs compact
   **payload** (`entry.compact = !req.m_return_legacy_full`).
3. Wallet `pull_blocks`:  
   `m_return_compact = true`,  
   `m_return_legacy_full = !m_compact_sync`.
4. Proxy `call_COMMAND_RPC_GET_BLOCKS_DIRECT` must forward `m_return_legacy_full`.

Do **not** copy Zano `m_client_version` / `PROJECT_VERSION_LONG` unless PDC
gains that field on the request struct (not present today).

## Intentionally not copying from Zano

- `bare_outputs_snapshot.cpp` bucket table (wrong chain) — keep PDC empty +
  `export_bare_outputs`.
- Rename `core_rpc_proxy.h` → `i_core_rpc_proxy.h`.

## Patches on disk

Reference diffs (upstream, for review):

- `docs/zano-backports/zano-772.patch`
- `docs/zano-backports/zano-770.patch` (already applied in tree)
- `docs/zano-backports/zano-769.patch` (already applied in tree)
