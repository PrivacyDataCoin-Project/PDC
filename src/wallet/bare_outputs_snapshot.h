// Copyright (c) 2014-2026 Zano Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <cstdint>
#include <unordered_map>

namespace currency
{
namespace bare_outputs_snapshot
{
  // Full amount-bucket sizes for THIS chain only: valid global indices are [0, count).
  // Empty map is intentional until regenerated via daemon `export_bare_outputs`
  // against a synced PDC node (never copy Zano or other-chain snapshots).
  //
  // An empty map (or a per-amount count too small for the ring) disables the
  // bare-candidate path for those amounts; the wallet then uses the
  // height-distribution path. See bare_candidates_viable().
  extern const std::unordered_map<std::uint64_t, std::uint64_t> bare_output_count_by_amount;
}
}
