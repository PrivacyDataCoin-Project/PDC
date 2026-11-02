// Copyright (c) 2014-2026 Zano Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <algorithm>
#include <cstdint>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "gtest/gtest.h"
#include "wallet/bare_decoy_batch_planner.h"
#include "wallet/bare_outputs_snapshot.h"

using tools::bare_decoy_batch_planner::append_bare_decoy_candidates;
using tools::bare_decoy_batch_planner::bare_candidates_viable;
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
      {2000000000000ULL, 500ULL},
      {100000ULL, 1ULL} // degenerate PDC bucket
    };
  }
}

// H1 guard: the embedded snapshot is the PDC-chain export (genesis
// df35cba5...00b10a, HF4 at block 100 so bare set is frozen), never another
// chain's counts. The 1e12 bucket holds 100 outputs; the other nine are
// degenerate (count == 1) and must be rejected by the gate.
TEST(bare_decoy_batch_planner, pdc_bare_outputs_snapshot_matches_pdc_export)
{
  const auto& m = currency::bare_outputs_snapshot::bare_output_count_by_amount;
  ASSERT_EQ(m.size(), 10u);
  EXPECT_EQ(m.at(1000000000000ULL), 100ULL);
  EXPECT_EQ(m.at(100000ULL), 1ULL);
  EXPECT_EQ(m.at(900000000000ULL), 1ULL);
  // every other bucket is degenerate on PDC (HF4 @ 100 freezes the bare set)
  for (const auto& kv : m)
    if (kv.first != 1000000000000ULL)
      EXPECT_EQ(kv.second, 1ULL) << "amount " << kv.first;
  // gate behaviour on the real snapshot: only the 1e12 bucket can serve a ring
  EXPECT_TRUE(bare_candidates_viable(1000000000000ULL, 99, m));
  EXPECT_FALSE(bare_candidates_viable(1000000000000ULL, 100, m));
  EXPECT_FALSE(bare_candidates_viable(100000ULL, 1, m));
}

// Gate: an empty map (placeholder snapshot) must NOT enable the bare path.
TEST(bare_decoy_batch_planner, gate_rejects_empty_map)
{
  const std::unordered_map<uint64_t, uint64_t> empty_counts;
  EXPECT_FALSE(bare_candidates_viable(1000000000000ULL, 8, empty_counts));
  EXPECT_FALSE(bare_candidates_viable(1000000000000ULL, 0, empty_counts));
}

// Gate: unknown amount is not viable even when other amounts are present.
TEST(bare_decoy_batch_planner, gate_rejects_unknown_amount)
{
  EXPECT_FALSE(bare_candidates_viable(12345ULL, 8, sample_counts()));
}

// Gate: pool must be able to hold real output + all decoys.
TEST(bare_decoy_batch_planner, gate_requires_ring_fits_pool)
{
  const auto counts = sample_counts(); // {1e12: 1000, 2e12: 500}
  EXPECT_TRUE(bare_candidates_viable(1000000000000ULL, 999, counts));   // 1000 >= 999+1
  EXPECT_FALSE(bare_candidates_viable(1000000000000ULL, 1000, counts));  // 1000 < 1000+1
  EXPECT_TRUE(bare_candidates_viable(2000000000000ULL, 499, counts));
  EXPECT_FALSE(bare_candidates_viable(2000000000000ULL, 500, counts));
}

// Gate: degenerate PDC buckets (count == 1) are never viable for a real ring.
TEST(bare_decoy_batch_planner, gate_rejects_count_one_buckets)
{
  const auto counts = sample_counts(); // {1e12: 1000, 2e12: 500, 100000: 1}
  EXPECT_FALSE(bare_candidates_viable(100000ULL, 1, counts));   // count 1 < 1+1
  EXPECT_FALSE(bare_candidates_viable(100000ULL, 10, counts));
  EXPECT_TRUE(bare_candidates_viable(1000000000000ULL, 999, counts));  // 1000 >= 1000
  EXPECT_FALSE(bare_candidates_viable(1000000000000ULL, 1000, counts)); // 1000 < 1001
}

// CASE 1 (big bucket): planner appends exactly decoy_count UNIQUE candidates,
// none of them the real gindex. The real output is added by the wallet, not here.
TEST(bare_decoy_batch_planner, big_bucket_appends_unique_decoys_without_real)
{
  request_batch batch;
  batch.input_amount = 1000000000000ULL; // count = 1000
  const uint64_t real_gindex = 42;
  const size_t decoy_count = 15;
  std::mt19937_64 rng(1);

  append_bare_decoy_candidates(batch, decoy_count, real_gindex, sample_counts(), k_max_bare_pool_size, rng);

  ASSERT_EQ(batch.ring_size, decoy_count + 1);
  ASSERT_EQ(batch.global_offsets.size(), decoy_count); // decoys only, no real
  std::unordered_set<uint64_t> uniq(batch.global_offsets.begin(), batch.global_offsets.end());
  EXPECT_EQ(uniq.size(), decoy_count); // all unique
  EXPECT_EQ(uniq.count(real_gindex), 0u); // real not among candidates
  for (uint64_t g : batch.global_offsets)
    EXPECT_LT(g, 1000ULL); // within the known bucket range
}

// Small-but-viable bucket: sampling must stay unique and exclude the real index
// even when the pool is only slightly larger than the ring (without-replacement).
TEST(bare_decoy_batch_planner, small_viable_bucket_unique_without_real)
{
  const std::unordered_map<uint64_t, uint64_t> counts = {{5000ULL, 17ULL}}; // ring 15+1 fits, pool small
  request_batch batch;
  batch.input_amount = 5000ULL;
  const uint64_t real_gindex = 3;
  const size_t decoy_count = 15;
  std::mt19937_64 rng(99);

  append_bare_decoy_candidates(batch, decoy_count, real_gindex, counts, k_max_bare_pool_size, rng);

  ASSERT_EQ(batch.global_offsets.size(), decoy_count);
  std::unordered_set<uint64_t> uniq(batch.global_offsets.begin(), batch.global_offsets.end());
  EXPECT_EQ(uniq.size(), decoy_count);
  EXPECT_EQ(uniq.count(real_gindex), 0u);
  for (uint64_t g : batch.global_offsets)
    EXPECT_LT(g, 17ULL);
}

// Small bucket that cannot hold the ring must throw instead of producing a bad pool.
TEST(bare_decoy_batch_planner, too_small_bucket_throws)
{
  const std::unordered_map<uint64_t, uint64_t> counts = {{5000ULL, 5ULL}}; // needs 16 for ring 15+1
  request_batch batch;
  batch.input_amount = 5000ULL;
  std::mt19937_64 rng(1);
  EXPECT_THROW(append_bare_decoy_candidates(batch, 15, 0, counts, k_max_bare_pool_size, rng), std::runtime_error);
}

// CASE 2 (small bucket): plan_batches_for_inputs self-gates and produces NO bare
// batch for the input whose bucket is too small -> the caller falls back to
// height-based selection for that input.
TEST(bare_decoy_batch_planner, plan_skips_bare_batch_when_bucket_too_small)
{
  const std::vector<input_plan_request> inputs = {
    {1000000000000ULL, 10, 15, true},  // viable (1000 >= 16) -> bare batch
    {100000ULL,       20, 15, true},   // count 1 -> skipped (height fallback)
    {12345ULL,        30, 15, true}    // unknown amount -> skipped (height fallback)
  };
  std::mt19937_64 rng(7);
  auto batches = plan_batches_for_inputs(inputs, sample_counts(), k_max_bare_pool_size, rng, true);

  ASSERT_EQ(batches.size(), 1u); // only the first input got a bare batch
  EXPECT_EQ(batches[0].input_amount, 1000000000000ULL);
  EXPECT_EQ(batches[0].ring_size, 16u);
  EXPECT_EQ(batches[0].global_offsets.size(), 15u); // decoys only
  EXPECT_EQ(std::count(batches[0].global_offsets.begin(), batches[0].global_offsets.end(), 10ULL), 0u);
}

// #771: shuffle still applies to the decoy candidates.
TEST(bare_decoy_batch_planner, shuffle_changes_decoy_order)
{
  const uint64_t amount = 1000000000000ULL;
  const uint64_t real_gindex = 7;
  const size_t decoy_count = 10;
  const std::vector<input_plan_request> inputs = {{amount, real_gindex, decoy_count, true}};

  std::mt19937_64 rng_unshuffled(12345);
  auto unshuffled = plan_batches_for_inputs(inputs, sample_counts(), k_max_bare_pool_size, rng_unshuffled, /*shuffle_bare_offsets=*/false);
  ASSERT_EQ(unshuffled.size(), 1u);
  ASSERT_EQ(unshuffled[0].global_offsets.size(), decoy_count);
  EXPECT_EQ(std::count(unshuffled[0].global_offsets.begin(), unshuffled[0].global_offsets.end(), real_gindex), 0u);

  std::mt19937_64 rng_shuffled(12345);
  auto shuffled = plan_batches_for_inputs(inputs, sample_counts(), k_max_bare_pool_size, rng_shuffled, /*shuffle_bare_offsets=*/true);
  ASSERT_EQ(shuffled.size(), 1u);
  ASSERT_EQ(shuffled[0].global_offsets.size(), decoy_count);
  EXPECT_NE(shuffled[0].global_offsets, unshuffled[0].global_offsets);
  EXPECT_EQ(std::count(shuffled[0].global_offsets.begin(), shuffled[0].global_offsets.end(), real_gindex), 0u);
}

// #773: N viable inputs with the same amount produce N separate bare batches.
TEST(bare_decoy_batch_planner, same_amount_gets_separate_batches_per_input)
{
  const uint64_t amount = 1000000000000ULL;
  const size_t decoy_count = 5;
  const std::vector<input_plan_request> inputs = {
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
    EXPECT_EQ(batches[i].global_offsets.size(), decoy_count); // decoys only, real added by wallet
    EXPECT_EQ(std::count(batches[i].global_offsets.begin(), batches[i].global_offsets.end(), inputs[i].real_gindex), 0u);
  }
}
