// Copyright (c) 2014-2026 Zano Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <algorithm>
#include <cstdint>
#include <unordered_map>
#include <vector>

#include "gtest/gtest.h"
#include "wallet/bare_decoy_batch_planner.h"
#include "wallet/bare_outputs_snapshot.h"

using tools::bare_decoy_batch_planner::append_bare_decoy_candidates;
using tools::bare_decoy_batch_planner::input_plan_request;
using tools::bare_decoy_batch_planner::plan_batches_for_inputs;
using tools::bare_decoy_batch_planner::request_batch;

namespace
{
  constexpr size_t k_max_bare_pool_size = 256 * (15 + 1) * 2;

  std::unordered_map<uint64_t, uint64_t> sample_counts()
  {
    return {
      {1000000000000ULL, 1000ULL},
      {2000000000000ULL, 500ULL}
    };
  }
}

// H1 guard: embedded snapshot must not carry another chain's (e.g. Zano) counts.
// Empty map is the safe PDC default until `export_bare_outputs` is run on PDC.
TEST(bare_decoy_batch_planner, pdc_bare_outputs_snapshot_is_empty_until_exported)
{
  EXPECT_TRUE(currency::bare_outputs_snapshot::bare_output_count_by_amount.empty());
}

// Empty snapshot must still produce a valid bare request (daemon resolves bounds).
TEST(bare_decoy_batch_planner, empty_count_map_still_appends_real_and_decoys)
{
  request_batch batch;
  batch.input_amount = 1000000000000ULL;
  const uint64_t real_gindex = 99;
  std::mt19937_64 rng(3);
  const std::unordered_map<uint64_t, uint64_t> empty_counts;
  append_bare_decoy_candidates(batch, 8, real_gindex, empty_counts, k_max_bare_pool_size, rng);
  ASSERT_EQ(batch.global_offsets.size(), 9u);
  EXPECT_EQ(batch.global_offsets.front(), real_gindex);
}

// #765/#771: real_gindex is always included; decoy_count additional candidates are requested.
TEST(bare_decoy_batch_planner, append_includes_real_and_decoy_count)
{
  request_batch batch;
  batch.input_amount = 1000000000000ULL;
  const uint64_t real_gindex = 42;
  const size_t decoy_count = 15;
  std::mt19937_64 rng(1);

  append_bare_decoy_candidates(batch, decoy_count, real_gindex, sample_counts(), k_max_bare_pool_size, rng);

  ASSERT_EQ(batch.ring_size, decoy_count + 1);
  ASSERT_EQ(batch.global_offsets.size(), decoy_count + 1);
  EXPECT_NE(std::find(batch.global_offsets.begin(), batch.global_offsets.end(), real_gindex), batch.global_offsets.end());
  EXPECT_EQ(batch.global_offsets.front(), real_gindex);
}

// #771: after planning, bare offsets are shuffled (fixed seed); real index remains present.
TEST(bare_decoy_batch_planner, shuffle_keeps_real_and_can_change_order)
{
  const uint64_t amount = 1000000000000ULL;
  const uint64_t real_gindex = 7;
  const size_t decoy_count = 10;

  std::vector<input_plan_request> inputs = {
    {amount, real_gindex, decoy_count, true}
  };

  std::mt19937_64 rng_unshuffled(12345);
  auto unshuffled = plan_batches_for_inputs(inputs, sample_counts(), k_max_bare_pool_size, rng_unshuffled, /*shuffle_bare_offsets=*/false);
  ASSERT_EQ(unshuffled.size(), 1u);
  ASSERT_FALSE(unshuffled[0].global_offsets.empty());
  EXPECT_EQ(unshuffled[0].global_offsets.front(), real_gindex);

  std::mt19937_64 rng_shuffled(12345);
  auto shuffled = plan_batches_for_inputs(inputs, sample_counts(), k_max_bare_pool_size, rng_shuffled, /*shuffle_bare_offsets=*/true);
  ASSERT_EQ(shuffled.size(), 1u);
  ASSERT_EQ(shuffled[0].global_offsets.size(), decoy_count + 1);
  EXPECT_NE(std::find(shuffled[0].global_offsets.begin(), shuffled[0].global_offsets.end(), real_gindex), shuffled[0].global_offsets.end());

  // Fixed seed: shuffle must change order relative to insertion (real first).
  EXPECT_NE(shuffled[0].global_offsets, unshuffled[0].global_offsets);
  // Not all seeds move the real off the front; require that at least one index moved.
  bool order_changed = false;
  for (size_t i = 0; i < shuffled[0].global_offsets.size(); ++i)
  {
    if (shuffled[0].global_offsets[i] != unshuffled[0].global_offsets[i])
    {
      order_changed = true;
      break;
    }
  }
  EXPECT_TRUE(order_changed);
}

// #773: N inputs with the same amount produce N separate batches.
TEST(bare_decoy_batch_planner, same_amount_gets_separate_batches_per_input)
{
  const uint64_t amount = 1000000000000ULL;
  const size_t decoy_count = 5;
  std::vector<input_plan_request> inputs = {
    {amount, 10, decoy_count, true},
    {amount, 20, decoy_count, true},
    {amount, 30, decoy_count, true}
  };

  std::mt19937_64 rng(7);
  auto batches = plan_batches_for_inputs(inputs, sample_counts(), k_max_bare_pool_size, rng, true);

  ASSERT_EQ(batches.size(), inputs.size());
  for (size_t i = 0; i < batches.size(); ++i)
  {
    EXPECT_EQ(batches[i].input_amount, amount);
    EXPECT_EQ(batches[i].ring_size, decoy_count + 1);
    EXPECT_EQ(batches[i].global_offsets.size(), decoy_count + 1);
    EXPECT_NE(std::find(batches[i].global_offsets.begin(), batches[i].global_offsets.end(), inputs[i].real_gindex),
              batches[i].global_offsets.end());
  }
}
