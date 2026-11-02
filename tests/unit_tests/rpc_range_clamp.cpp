// Copyright (c) 2014-2024 Zano Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <cstdint>
#include <limits>

#include "gtest/gtest.h"

#include "currency_core/blockchain_storage.h"

TEST(clamp_rpc_range_count, leaves_in_range_window)
{
  EXPECT_EQ(3u, currency::clamp_rpc_range_count(2, 3, 10));
}

TEST(clamp_rpc_range_count, saturates_at_end)
{
  EXPECT_EQ(4u, currency::clamp_rpc_range_count(6, 100, 10));
  EXPECT_EQ(10u, currency::clamp_rpc_range_count(0, 10, 10));
}

TEST(clamp_rpc_range_count, empty_when_offset_past_end_or_count_zero)
{
  EXPECT_EQ(0u, currency::clamp_rpc_range_count(10, 1, 10));
  EXPECT_EQ(0u, currency::clamp_rpc_range_count(0, 0, 10));
}

TEST(clamp_rpc_range_count, does_not_wrap_uint64_sum)
{
  const uint64_t size = 10;
  const uint64_t offset = 1;
  const uint64_t count = std::numeric_limits<uint64_t>::max();
  EXPECT_EQ(size - offset, currency::clamp_rpc_range_count(offset, count, size));
}
