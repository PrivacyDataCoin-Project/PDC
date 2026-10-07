// Copyright (c) 2014-2024 Zano Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <cstdint>

namespace currency
{
  // Window of the last blocks_limit blocks, clamped to the chain that exists.
  // For top_height + 1 >= blocks_limit the start offset matches
  // top_height - blocks_limit + 1, which is the 2.3.1 formula.
  inline void recent_blocks_window(uint64_t top_height, uint64_t blocks_limit, uint64_t& start_offset, uint64_t& count)
  {
    count = blocks_limit;
    if (count > top_height + 1)
      count = top_height + 1;
    start_offset = top_height + 1 - count;
  }
}
