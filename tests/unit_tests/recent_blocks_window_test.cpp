// Copyright (c) 2014-2024 Zano Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "gtest/gtest.h"
#include "rpc/recent_blocks_window.h"

TEST(recent_blocks_window, tall_chain_matches_release_formula)
{
  uint64_t start = 0;
  uint64_t count = 0;
  currency::recent_blocks_window(10, 5, start, count);
  EXPECT_EQ(6u, start);
  EXPECT_EQ(5u, count);
}

TEST(recent_blocks_window, short_chain_starts_at_genesis)
{
  uint64_t start = 99;
  uint64_t count = 99;
  currency::recent_blocks_window(0, 5, start, count);
  EXPECT_EQ(0u, start);
  EXPECT_EQ(1u, count);

  currency::recent_blocks_window(3, 5, start, count);
  EXPECT_EQ(0u, start);
  EXPECT_EQ(4u, count);
}
