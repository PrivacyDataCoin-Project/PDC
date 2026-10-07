// Copyright (c) 2014-2018 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "checkpoints.h"
#include "currency_config.h"
#include "misc_log_ex.h"

#define ADD_CHECKPOINT(h, hash)  CHECK_AND_ASSERT(checkpoints.add_checkpoint(h,  hash), false)

namespace currency
{

  inline bool create_checkpoints(currency::checkpoints& checkpoints)
  {
#ifdef TESTNET

#else
    // MAINNET
    // Block ids every 1000 heights through the last complete thousand.
    // Public seed 169.58.142.131 reported chain size 12336 (top height 12335).
    ADD_CHECKPOINT(0,     CURRENCY_GENESIS_HASH);
    ADD_CHECKPOINT(1000,  "c76d938cf1460aee1b0ed63ddd707bda4c79232a701236c887a57e28c9082e9b");
    ADD_CHECKPOINT(2000,  "6fb052670513f52797ef372e445562187e0c80d5a76efa8efffc1b461816914b");
    ADD_CHECKPOINT(3000,  "d3c22a8a55ba145fc0f41cb659d97c7ae537d971c88517878314fbae4258c6c2");
    ADD_CHECKPOINT(4000,  "57523126d29e7e7ffea8734b26c4ccc0629a2413652d55730a78eda50125a7a7");
    ADD_CHECKPOINT(5000,  "15ede88fc9cccb684b8c84fcd547934cc7ec09187e84b395526b9f07ebad925b");
    ADD_CHECKPOINT(6000,  "9ce6204498bfa2b250c700577f284030c4f3c0c13abfaaec91cb0cbd8f3f6c2e");
    ADD_CHECKPOINT(7000,  "97a24464b8d2a9d32a202df9830749700079a5e68b5e9d954b4d469702599244");
    ADD_CHECKPOINT(8000,  "20ad9c2ef2d9a9ded3e7275d61f710ba3d1840031ed3045c9da428e3851e8697");
    ADD_CHECKPOINT(9000,  "73b6ad5abeb07cf70b7ea4219eea54b54a75d57a39c4f64741409d1e730f93be");
    ADD_CHECKPOINT(10000, "a3cf217b2b130f17bf2e8d63d03425f04b13d052e5650975546841190949096d");
    ADD_CHECKPOINT(11000, "f7c75c967b3eab53e6d78097eda083b344acc31c93795bed554ee97f12e8fb15");
    ADD_CHECKPOINT(12000, "94ab6b62aae2ac799aa25650eab234518b21a5ea0cd1d0fd995fd2a70caac6f6");
#endif

    return true;
  }

} // namespace currency 
