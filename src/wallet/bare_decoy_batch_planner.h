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
#include <utility>
#include <vector>

namespace tools
{
namespace bare_decoy_batch_planner
{
  // Pure planning helpers mirroring wallet2 bare-decoy batch construction
  // (AMOUNTS4 / PRs #765, #771, #773) for deterministic unit tests.

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

  inline void append_bare_decoy_candidates(request_batch& batch,
                                          size_t decoy_count,
                                          uint64_t real_gindex,
                                          const std::unordered_map<uint64_t, uint64_t>& bare_output_count_by_amount,
                                          size_t max_bare_pool_size,
                                          std::mt19937_64& rng)
  {
    if (decoy_count >= max_bare_pool_size)
      throw std::runtime_error("Requested bare ring exceeds the daemon output limit");

    uint64_t upper_index = std::numeric_limits<uint64_t>::max();
    const auto count_it = bare_output_count_by_amount.find(batch.input_amount);
    if (count_it != bare_output_count_by_amount.end() && count_it->second != 0)
      upper_index = count_it->second - 1;

    std::uniform_int_distribution<uint64_t> position(0, upper_index);
    const size_t ring_size = decoy_count + 1;
    if (batch.global_offsets.size() > max_bare_pool_size - ring_size)
      throw std::runtime_error("Requested bare pool exceeds the daemon output limit");

    batch.ring_size = std::max(batch.ring_size, uint64_t(ring_size));

    // #765/#771: include the real output, then request decoy_count additional candidates
    batch.global_offsets.push_back(real_gindex);
    for (size_t j = 0; j < decoy_count; ++j)
      batch.global_offsets.push_back(position(rng));
  }

  // One batch per input (#773). Same amount does not share a batch.
  inline std::vector<request_batch> plan_batches_for_inputs(const std::vector<input_plan_request>& inputs,
                                                           const std::unordered_map<uint64_t, uint64_t>& bare_output_count_by_amount,
                                                           size_t max_bare_pool_size,
                                                           std::mt19937_64& rng,
                                                           bool shuffle_bare_offsets)
  {
    std::vector<request_batch> batches;
    batches.reserve(inputs.size());

    for (const auto& in : inputs)
    {
      if (!in.use_bare_candidates || in.decoy_count == 0)
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
