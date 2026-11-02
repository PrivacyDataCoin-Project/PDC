// Copyright (c) 2014-2026 Zano Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <algorithm>
#include <cstdint>
#include <limits>
#include <random>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace tools
{
namespace bare_decoy_batch_planner
{
  // Pure planning helpers mirroring wallet2 bare-decoy batch construction
  // (AMOUNTS4 / PRs #765, #771, #773) for deterministic unit tests.

  // Decide whether the bare-candidate path may be used for one input.
  //
  // Bare candidates are only safe when the embedded snapshot carries a real count
  // for THIS amount AND that pool can actually satisfy the requested ring (the
  // real output plus decoy_count decoys). Any other case must fall back to the
  // height-distribution path:
  //   * empty map   - the snapshot was never exported for this chain, so the
  //                   daemon would reduce random uint64 candidates modulo the
  //                   real count (unpredictable, privacy-relevant);
  //   * count < ring - the pool is too small, the daemon returns fewer outputs
  //                   than required and the old code hard-threw instead of
  //                   falling back.
  // A count of 1 (several PDC bare buckets are like this) therefore correctly
  // disables the bare path for those amounts.
  inline bool bare_candidates_viable(uint64_t amount,
                                     size_t target_decoy_count,
                                     const std::unordered_map<uint64_t, uint64_t>& bare_output_count_by_amount)
  {
    if (target_decoy_count == 0)
      return false;
    const auto count_it = bare_output_count_by_amount.find(amount);
    if (count_it == bare_output_count_by_amount.end() || count_it->second == 0)
      return false;
    return count_it->second >= target_decoy_count + 1; // ring = real output + decoys
  }

  struct request_batch
  {
    uint64_t input_amount = 0;
    std::vector<uint64_t> heights;
    std::vector<uint64_t> global_offsets;
    uint64_t ring_size = 0;
  };

  struct input_plan_request
  {
    uint64_t amount = 0;
    uint64_t real_gindex = 0;
    size_t decoy_count = 0;
    bool use_bare_candidates = false;
  };

  // Append decoy_count UNIQUE candidate indices, none of which equals real_gindex.
  // The real output is NOT part of global_offsets: wallet2 inserts it into the ring
  // itself (distribute_decoys_and_build_sources), and the daemon requires the real
  // gindex to be present in its own response pool. Keeping it out of the candidate
  // list also guarantees uniqueness of what we request.
  // Rng must satisfy the UniformRandomBitGenerator concept (e.g. std::mt19937_64
  // in tests, crypto::uniform_random_bit_generator in the wallet - matching Zano).
  template <typename Rng>
  inline void append_bare_decoy_candidates(request_batch& batch,
                                          size_t decoy_count,
                                          uint64_t real_gindex,
                                          const std::unordered_map<uint64_t, uint64_t>& bare_output_count_by_amount,
                                          size_t max_bare_pool_size,
                                          Rng& rng)
  {
    if (decoy_count >= max_bare_pool_size)
      throw std::runtime_error("Requested bare ring exceeds the daemon output limit");

    // Missing/zero count => full uint64 range. The daemon maps candidates with
    // outputs_count (modulo + walk). Callers are expected to gate on
    // bare_candidates_viable() first; this fallback is kept only so the helper
    // stays total. Prefer an empty map over foreign-chain counts.
    uint64_t upper_index = std::numeric_limits<uint64_t>::max();
    const auto count_it = bare_output_count_by_amount.find(batch.input_amount);
    if (count_it != bare_output_count_by_amount.end() && count_it->second != 0)
      upper_index = count_it->second - 1;

    // Candidates must fit in [0, count) and exclude the real output, so the pool
    // needs at least decoy_count + 1 indices in total.
    if (upper_index != std::numeric_limits<uint64_t>::max() && upper_index + 1 < uint64_t(decoy_count) + 1)
      throw std::runtime_error("Bare output pool is too small for the requested ring");

    std::uniform_int_distribution<uint64_t> position(0, upper_index);
    const size_t ring_size = decoy_count + 1;
    if (batch.global_offsets.size() > max_bare_pool_size - decoy_count)
      throw std::runtime_error("Requested bare pool exceeds the daemon output limit");

    batch.ring_size = std::max(batch.ring_size, uint64_t(ring_size));

    // Request only decoys: unique, and never the real gindex. When the known pool
    // is small, draw without replacement via a set; otherwise redraw on collision
    // (the daemon also de-duplicates modulo collisions, but we must not ask for
    // the real output).
    const bool small_pool = upper_index != std::numeric_limits<uint64_t>::max()
                            && upper_index < uint64_t(decoy_count) * 8;
    std::unordered_set<uint64_t> seen;
    if (small_pool)
      seen.reserve(decoy_count);
    for (size_t j = 0; j < decoy_count; ++j)
    {
      uint64_t candidate = 0;
      if (small_pool)
      {
        do { candidate = position(rng); } while (candidate == real_gindex || !seen.insert(candidate).second);
      }
      else
      {
        do { candidate = position(rng); } while (candidate == real_gindex);
        // large pool: collision probability is negligible; do not track a set
      }
      batch.global_offsets.push_back(candidate);
    }
  }

  // One batch per input (#773). Same amount does not share a batch.
  template <typename Rng>
  inline std::vector<request_batch> plan_batches_for_inputs(const std::vector<input_plan_request>& inputs,
                                                           const std::unordered_map<uint64_t, uint64_t>& bare_output_count_by_amount,
                                                           size_t max_bare_pool_size,
                                                           Rng& rng,
                                                           bool shuffle_bare_offsets)
  {
    std::vector<request_batch> batches;
    batches.reserve(inputs.size());

    for (const auto& in : inputs)
    {
      // Self-gate as well: even if the caller asked for bare candidates, a missing
      // or too-small bucket must not produce a bare batch (the caller then falls
      // back to height-based selection for this input).
      if (!in.use_bare_candidates || in.decoy_count == 0
          || !bare_candidates_viable(in.amount, in.decoy_count, bare_output_count_by_amount))
        continue;

      request_batch batch;
      batch.input_amount = in.amount;
      append_bare_decoy_candidates(batch, in.decoy_count, in.real_gindex, bare_output_count_by_amount, max_bare_pool_size, rng);
      batches.push_back(std::move(batch));
    }

    if (shuffle_bare_offsets)
    {
      for (auto& batch : batches)
      {
        if (batch.global_offsets.empty())
          continue;
        // #771: shuffle so the real gindex is not at a fixed position
        std::shuffle(batch.global_offsets.begin(), batch.global_offsets.end(), rng);
      }
    }

    return batches;
  }
}
}
