// Copyright (c) 2026 Zano Project
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "web_channel_bridge.h"

#include "mainwindow.h"

WebChannelBridge::WebChannelBridge(MainWindow& main_window, QObject* parent)
  : QObject(parent)
  , m_main_window(main_window)
{
  setObjectName(QStringLiteral("web_channel_bridge"));

  // Prefer explicit emit over signal-to-signal so QWebChannel always sees
  // notifications on the registered mediator object (some Qt5 builds drop
  // signal-to-signal forwarding to the channel).
  connect(&m_main_window, &MainWindow::quit_requested, this,
    [this](const QString& str) { emit quit_requested(str); });
  connect(&m_main_window, &MainWindow::update_daemon_state, this,
    [this](const QString& str) { emit update_daemon_state(str); });
  connect(&m_main_window,
    static_cast<void (MainWindow::*)(QString)>(&MainWindow::update_wallet_status), this,
    [this](const QString& str) { emit update_wallet_status(str); });
  connect(&m_main_window, &MainWindow::update_wallet_info, this,
    [this](const QString& str) { emit update_wallet_info(str); });
  connect(&m_main_window,
    static_cast<void (MainWindow::*)(QString)>(&MainWindow::money_transfer), this,
    [this](const QString& str) { emit money_transfer(str); });
  connect(&m_main_window,
    static_cast<void (MainWindow::*)(QString)>(&MainWindow::money_transfer_cancel), this,
    [this](const QString& str) { emit money_transfer_cancel(str); });
  connect(&m_main_window,
    static_cast<void (MainWindow::*)(QString)>(&MainWindow::wallet_sync_progress), this,
    [this](const QString& str) { emit wallet_sync_progress(str); });
  connect(&m_main_window, &MainWindow::handle_internal_callback, this,
    [this](const QString& str, const QString& callback_name) {
      emit handle_internal_callback(str, callback_name);
    });
  connect(&m_main_window, &MainWindow::update_pos_mining_text, this,
    [this](const QString& str) { emit update_pos_mining_text(str); });
  connect(&m_main_window,
    static_cast<void (MainWindow::*)(QString)>(&MainWindow::on_core_event), this,
    [this](const QString& str) { emit on_core_event(str); });
  connect(&m_main_window,
    static_cast<void (MainWindow::*)(QString)>(&MainWindow::set_options), this,
    [this](const QString& str) { emit set_options(str); });
  connect(&m_main_window, &MainWindow::handle_deeplink_click, this,
    [this](const QString& str) { emit handle_deeplink_click(str); });
  connect(&m_main_window, &MainWindow::handle_current_action_state, this,
    [this](const QString& str) { emit handle_current_action_state(str); });
  connect(&m_main_window, &MainWindow::dispatch_async_call_result, this,
    [this](const QString& id, const QString& resp) {
      emit dispatch_async_call_result(id, resp);
    });
}

#define PDC_FORWARD_QSTRING_1(method_name) \
  QString WebChannelBridge::method_name(const QString& value) \
  { \
    return m_main_window.method_name(value); \
  }

PDC_FORWARD_QSTRING_1(show_openfile_dialog)
PDC_FORWARD_QSTRING_1(show_savefile_dialog)
PDC_FORWARD_QSTRING_1(open_wallet)
PDC_FORWARD_QSTRING_1(get_my_offers)
PDC_FORWARD_QSTRING_1(get_fav_offers)
PDC_FORWARD_QSTRING_1(generate_wallet)
PDC_FORWARD_QSTRING_1(run_wallet)
PDC_FORWARD_QSTRING_1(close_wallet)
PDC_FORWARD_QSTRING_1(get_contracts)
PDC_FORWARD_QSTRING_1(create_proposal)
PDC_FORWARD_QSTRING_1(accept_proposal)
PDC_FORWARD_QSTRING_1(release_contract)
PDC_FORWARD_QSTRING_1(request_cancel_contract)
PDC_FORWARD_QSTRING_1(accept_cancel_contract)
PDC_FORWARD_QSTRING_1(on_request_quit)
PDC_FORWARD_QSTRING_1(get_version)
PDC_FORWARD_QSTRING_1(get_os_version)
PDC_FORWARD_QSTRING_1(get_network_type)
PDC_FORWARD_QSTRING_1(transfer)
PDC_FORWARD_QSTRING_1(have_secure_app_data)
PDC_FORWARD_QSTRING_1(get_secure_app_data)
PDC_FORWARD_QSTRING_1(set_master_password)
PDC_FORWARD_QSTRING_1(check_master_password)
PDC_FORWARD_QSTRING_1(get_app_data)
PDC_FORWARD_QSTRING_1(store_app_data)
PDC_FORWARD_QSTRING_1(get_default_user_dir)
PDC_FORWARD_QSTRING_1(get_offers_ex)
PDC_FORWARD_QSTRING_1(push_offer)
PDC_FORWARD_QSTRING_1(cancel_offer)
PDC_FORWARD_QSTRING_1(push_update_offer)
PDC_FORWARD_QSTRING_1(get_alias_info_by_address)
PDC_FORWARD_QSTRING_1(get_alias_info_by_name)
PDC_FORWARD_QSTRING_1(get_all_aliases)
PDC_FORWARD_QSTRING_1(request_alias_registration)
PDC_FORWARD_QSTRING_1(request_alias_update)
PDC_FORWARD_QSTRING_1(get_alias_coast)
PDC_FORWARD_QSTRING_1(validate_address)
PDC_FORWARD_QSTRING_1(resync_wallet)
PDC_FORWARD_QSTRING_1(get_recent_transfers)
PDC_FORWARD_QSTRING_1(get_mining_history)
PDC_FORWARD_QSTRING_1(start_pos_mining)
PDC_FORWARD_QSTRING_1(stop_pos_mining)
PDC_FORWARD_QSTRING_1(set_log_level)
PDC_FORWARD_QSTRING_1(get_log_level)
PDC_FORWARD_QSTRING_1(get_log_files_size)
PDC_FORWARD_QSTRING_1(clear_log_files)
PDC_FORWARD_QSTRING_1(set_enable_tor)
PDC_FORWARD_QSTRING_1(webkit_launched_script)
PDC_FORWARD_QSTRING_1(get_smart_wallet_info)
PDC_FORWARD_QSTRING_1(restore_wallet)
PDC_FORWARD_QSTRING_1(use_whitelisting)
PDC_FORWARD_QSTRING_1(is_pos_allowed)
PDC_FORWARD_QSTRING_1(load_from_file)
PDC_FORWARD_QSTRING_1(is_file_exist)
PDC_FORWARD_QSTRING_1(get_mining_estimate)
PDC_FORWARD_QSTRING_1(backup_wallet_keys)
PDC_FORWARD_QSTRING_1(reset_wallet_password)
PDC_FORWARD_QSTRING_1(is_wallet_password_valid)
PDC_FORWARD_QSTRING_1(is_autostart_enabled)
PDC_FORWARD_QSTRING_1(toggle_autostart)
PDC_FORWARD_QSTRING_1(is_valid_restore_wallet_text)
PDC_FORWARD_QSTRING_1(get_seed_phrase_info)
PDC_FORWARD_QSTRING_1(print_text)
PDC_FORWARD_QSTRING_1(print_log)
PDC_FORWARD_QSTRING_1(set_clipboard)
PDC_FORWARD_QSTRING_1(get_clipboard)
PDC_FORWARD_QSTRING_1(get_exchange_last_top)
PDC_FORWARD_QSTRING_1(get_tx_pool_info)
PDC_FORWARD_QSTRING_1(get_default_fee)
PDC_FORWARD_QSTRING_1(get_options)
PDC_FORWARD_QSTRING_1(add_custom_asset_id)
PDC_FORWARD_QSTRING_1(remove_custom_asset_id)
PDC_FORWARD_QSTRING_1(get_wallet_info)
PDC_FORWARD_QSTRING_1(create_ionic_swap_proposal)
PDC_FORWARD_QSTRING_1(get_ionic_swap_proposal_info)
PDC_FORWARD_QSTRING_1(accept_ionic_swap_proposal)
PDC_FORWARD_QSTRING_1(export_wallet_history)
PDC_FORWARD_QSTRING_1(get_log_file)
PDC_FORWARD_QSTRING_1(open_url_in_browser)
PDC_FORWARD_QSTRING_1(setup_jwt_wallet_rpc)
PDC_FORWARD_QSTRING_1(is_remnotenode_mode_preconfigured)
PDC_FORWARD_QSTRING_1(start_backend)
PDC_FORWARD_QSTRING_1(request_dummy)
PDC_FORWARD_QSTRING_1(call_rpc)

#undef PDC_FORWARD_QSTRING_1

QString WebChannelBridge::drop_secure_app_data()
{
  return m_main_window.drop_secure_app_data();
}

QString WebChannelBridge::store_secure_app_data(const QString& param, const QString& password)
{
  return m_main_window.store_secure_app_data(param, password);
}

QString WebChannelBridge::store_to_file(const QString& path, const QString& buff)
{
  return m_main_window.store_to_file(path, buff);
}

QString WebChannelBridge::set_localization_strings(const QString str)
{
  return m_main_window.set_localization_strings(str);
}

void WebChannelBridge::message_box(const QString& msg)
{
  m_main_window.message_box(msg);
}

bool WebChannelBridge::toggle_mining(const QString& param)
{
  return m_main_window.toggle_mining(param);
}

void WebChannelBridge::bool_toggle_icon(const QString& param)
{
  m_main_window.bool_toggle_icon(param);
}

bool WebChannelBridge::get_is_disabled_notifications(const QString& param)
{
  return m_main_window.get_is_disabled_notifications(param);
}

bool WebChannelBridge::set_is_disabled_notifications(const bool& param)
{
  return m_main_window.set_is_disabled_notifications(param);
}

void WebChannelBridge::trayIconActivated(QSystemTrayIcon::ActivationReason reason)
{
  m_main_window.trayIconActivated(reason);
}

void WebChannelBridge::tray_quit_requested(const QString& param)
{
  m_main_window.tray_quit_requested(param);
}

void WebChannelBridge::on_menu_show(const QString& param)
{
  m_main_window.on_menu_show(param);
}

void WebChannelBridge::show_notification(const QString& title, const QString& message)
{
  m_main_window.show_notification(title, message);
}

QString WebChannelBridge::async_call(const QString& func_name, const QString& params)
{
  return m_main_window.async_call(func_name, params);
}

QString WebChannelBridge::sync_call(const QString& func_name, const QString& params)
{
  return m_main_window.sync_call(func_name, params);
}

QString WebChannelBridge::async_call_2a(const QString& func_name, const QString& params1, const QString& params2)
{
  return m_main_window.async_call_2a(func_name, params1, params2);
}

QString WebChannelBridge::sync_call_2a(const QString& func_name, const QString& params1, const QString& params2)
{
  return m_main_window.sync_call_2a(func_name, params1, params2);
}

QString WebChannelBridge::call_wallet_rpc(const QString& wallet_id, const QString& params)
{
  return m_main_window.call_wallet_rpc(wallet_id, params);
}
