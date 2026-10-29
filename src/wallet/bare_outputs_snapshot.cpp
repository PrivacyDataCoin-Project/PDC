// Copyright (c) 2014-2026 Zano Project
// Copyright (c) 2024-2026 PrivacyDataCoin Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

// PDC bare-output amount bucket sizes for wallet-side candidate sampling bounds.
//
// CRITICAL: Do NOT embed another chain's counts (e.g. Zano mainnet). Wrong counts
// bias bare decoy sampling toward a truncated index range and weaken privacy.
//
// Empty map = safe fallback: planner uses upper_index = UINT64_MAX and the daemon
// reduces candidates with outputs_count (modulo + walk) in get_bare_outs_by_indices.
//
// After a synced PDC node past HF4, regenerate this file:
//   export_bare_outputs src/wallet/bare_outputs_snapshot.cpp
// then replace this placeholder. Expected genesis must match CURRENCY_GENESIS_HASH
// (df35cba557c857756f20612ce3c9d2aa315d0ae8fc2aaffe6c5c59d37e00b10a).

#include "bare_outputs_snapshot.h"

namespace currency { namespace bare_outputs_snapshot {
extern const std::unordered_map<std::uint64_t, std::uint64_t> bare_output_count_by_amount = {
  // Intentionally empty until a PDC-chain export is installed.
};
}} // namespace currency::bare_outputs_snapshot
