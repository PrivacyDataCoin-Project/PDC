// Copyright (c) 2026 PrivacyDataCoin Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once
#include "chaingen.h"
#include "wallet_tests_basic.h"

struct native_pdc_fee_burn : public wallet_test
{
  native_pdc_fee_burn();
  bool generate(std::vector<test_event_entry>& events) const;
  bool c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
};
