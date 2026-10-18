// Copyright (c) 2026 PrivacyDataCoin Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "chaingen.h"
#include "native_fee_burn.h"

using namespace currency;

namespace
{
// Core tests do not contain the mainnet maintainer secret. This wallet still runs
// wallet2::burn_native, which derives a pubkey from the supplied secret and compares
// it. The override only replaces the expected pubkey; a mismatch is still rejected.
struct native_burn_test_wallet : public tools::wallet2
{
  bool substitute_authority = false;
  crypto::public_key substituted_authority = currency::null_pkey;

  crypto::public_key current_authority_pubkey() const
  {
    return native_burn_authority_pubkey();
  }

protected:
  crypto::public_key native_burn_authority_pubkey() const override
  {
    if (substitute_authority)
      return substituted_authority;
    return tools::wallet2::native_burn_authority_pubkey();
  }
};

bool native_burn_rejected(tools::wallet2& w, uint64_t amount, const crypto::secret_key& key)
{
  transaction tx = AUTO_VAL_INIT(tx);
  try
  {
    w.burn_native(amount, key, tx);
  }
  catch (const std::exception& e)
  {
    const std::string msg = e.what();
    return msg.find("P2P_MAINTAINERS_MAINNET") != std::string::npos || msg.find("maintainer key") != std::string::npos;
  }
  return false;
}
}

native_pdc_fee_burn::native_pdc_fee_burn()
{
  REGISTER_CALLBACK_METHOD(native_pdc_fee_burn, c1);
}

bool native_pdc_fee_burn::generate(std::vector<test_event_entry>& events) const
{
  m_accounts.resize(TOTAL_ACCS_COUNT);
  account_base& miner_acc = m_accounts[MINER_ACC_IDX];
  miner_acc.generate();

  // Keep the whole chain behind the core clock. Each rewind block steps the timestamp forward.
  uint64_t ts_start = test_core_time::get_time() - 120 * DIFFICULTY_POW_TARGET;
  MAKE_GENESIS_BLOCK(events, blk_0, miner_acc, ts_start);
  DO_CALLBACK(events, "configure_core");
  // Long enough that the oldest full-reward outputs satisfy HF4 coinage when spent.
  REWIND_BLOCKS_N(events, blk_0r, blk_0, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 80);
  DO_CALLBACK(events, "c1");
  return true;
}

bool native_pdc_fee_burn::c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  std::shared_ptr<native_burn_test_wallet> miner_wlt = init_playtime_test_wallet_t<native_burn_test_wallet>(events, c, MINER_ACC_IDX);
  CHECK_AND_ASSERT_MES(miner_wlt, false, "init_playtime_test_wallet failed");
  miner_wlt->set_core_runtime_config(c.get_blockchain_storage().get_core_runtime_config());
  miner_wlt->refresh();

  uint64_t unlocked = 0, awaiting_in = 0, awaiting_out = 0, mined = 0;
  uint64_t total_before = miner_wlt->balance(unlocked, awaiting_in, awaiting_out, mined);
  // Above the minimum fee. Small enough that selection uses the oldest outputs,
  // which already satisfy HF4 coinage. The wallet fee cap is bypassed by allow_high_fee
  // for any larger amount the wallet can select from sufficiently old outputs.
  const uint64_t burn = 10 * COIN;
  CHECK_AND_ASSERT_MES(unlocked > burn, false, "not enough unlocked coins for the burn test: unlocked="
    << print_money_brief(unlocked) << ", burn=" << print_money_brief(burn));

  CHECK_AND_ASSERT_MES(epee::string_tools::pod_to_hex(miner_wlt->current_authority_pubkey()) == P2P_MAINTAINERS_MAINNET, false,
    "native burn authority is not P2P_MAINTAINERS_MAINNET");
  CHECK_AND_ASSERT_MES(native_burn_rejected(*miner_wlt, burn, null_skey), false, "missing maintainer key was accepted");

  crypto::public_key test_pub = AUTO_VAL_INIT(test_pub);
  crypto::secret_key test_sec = AUTO_VAL_INIT(test_sec);
  crypto::generate_keys(test_pub, test_sec);
  crypto::public_key other_pub = AUTO_VAL_INIT(other_pub);
  crypto::secret_key other_sec = AUTO_VAL_INIT(other_sec);
  crypto::generate_keys(other_pub, other_sec);

  CHECK_AND_ASSERT_MES(native_burn_rejected(*miner_wlt, burn, test_sec), false, "unrelated key was accepted against P2P_MAINTAINERS_MAINNET");
  transaction rejected_tx = AUTO_VAL_INIT(rejected_tx);
  bool asset_entry_rejected = false;
  try
  {
    miner_wlt->burn_asset(native_coin_asset_id, burn, rejected_tx);
  }
  catch (const std::exception&)
  {
    asset_entry_rejected = true;
  }
  CHECK_AND_ASSERT_MES(asset_entry_rejected, false, "burn_asset(native) without maintainer key was accepted");

  miner_wlt->substituted_authority = test_pub;
  miner_wlt->substitute_authority = true;
  CHECK_AND_ASSERT_MES(native_burn_rejected(*miner_wlt, burn, other_sec), false, "pubkey mismatch was accepted");
  CHECK_AND_ASSERT_MES(native_burn_rejected(*miner_wlt, burn, null_skey), false, "missing key was accepted after the test hook");

  boost::multiprecision::uint128_t emission_before = c.get_blockchain_storage().total_coins();

  transaction tx = AUTO_VAL_INIT(tx);
  miner_wlt->burn_native(burn, test_sec, tx);

  uint64_t fee = 0;
  CHECK_AND_ASSERT_MES(get_tx_fee(tx, fee), false, "burn tx has no explicit fee");
  CHECK_AND_ASSERT_MES(fee == burn, false, "burn tx fee " << print_money_brief(fee) << " != requested burn " << print_money_brief(burn));

  asset_descriptor_operation ado = AUTO_VAL_INIT(ado);
  CHECK_AND_ASSERT_MES(!get_type_in_variant_container(tx.extra, ado), false, "native burn must not carry PUBLIC_BURN");

  bool r = mine_next_pow_block_in_playtime(miner_wlt->get_account().get_public_address(), c);
  CHECK_AND_ASSERT_MES(r, false, "mine_next_pow_block_in_playtime failed");

  block top{};
  r = c.get_blockchain_storage().get_top_block(top);
  CHECK_AND_ASSERT_MES(r, false, "get_top_block failed");
  const crypto::hash tx_id = get_transaction_hash(tx);
  bool included = false;
  for (const auto& id : top.tx_hashes)
  {
    if (id == tx_id)
      included = true;
  }
  CHECK_AND_ASSERT_MES(included, false, "burn tx was not included in the next block");

  uint64_t reward = get_outs_money_amount(top.miner_tx, miner_wlt->get_account().get_keys());
  boost::multiprecision::uint128_t emission_after = c.get_blockchain_storage().total_coins();
  CHECK_AND_ASSERT_MES(emission_after + burn == emission_before + reward, false,
    "emission was not reduced by the burned fee");

  miner_wlt->refresh();
  uint64_t total_after = miner_wlt->balance();
  // The miner also receives this block's reward, so the wallet drops by the burn and gains the reward.
  CHECK_AND_ASSERT_MES(total_after + burn == total_before + reward, false,
    "wallet total " << print_money_brief(total_after) << " expected " << print_money_brief(total_before + reward - burn));

  return true;
}
