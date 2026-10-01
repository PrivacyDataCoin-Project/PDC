#!/bin/bash
script_dir=$( dirname "$(readlink -f "$0")" )

# Bundle Qt/OpenSSL first so system OpenSSL 3 cannot satisfy Qt 5.12 SSL symbols.
export LD_LIBRARY_PATH="$script_dir/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
export QT_PLUGIN_PATH="$script_dir/lib"
export QT_QPA_PLATFORM_PLUGIN_PATH="$script_dir/lib/platforms"

# Relocate WebEngine away from the CI Qt prefix baked into libQt5WebEngineCore.
export QTWEBENGINEPROCESS_PATH="$script_dir/QtWebEngineProcess"
export QTWEBENGINE_DISABLE_SANDBOX=1
export QTWEBENGINE_CHROMIUM_FLAGS="${QTWEBENGINE_CHROMIUM_FLAGS:---disable-gpu --no-sandbox}"

out_file_name=~/.local/share/applications/Pdc.desktop

call_app()
{
  pushd "$script_dir" >/dev/null
  ./Pdc "$@"
  status=$?
  popd >/dev/null
  exit $status
}


create_desktop_icon()
{
    target_file_name=$1
    echo "Generating icon file: $target_file_name..."
    rm -f $target_file_name
    echo [Desktop Entry] | tee -a $target_file_name  > /dev/null
    echo Version=1.0 | tee -a $target_file_name  > /dev/null
    echo Name=Pdc | tee -a $target_file_name > /dev/null
    echo GenericName=Pdc | tee -a $target_file_name  > /dev/null
    echo Comment=Privacy blockchain | tee -a $target_file_name  > /dev/null
    echo Icon=$script_dir/html/files/desktop_linux_icon.png | tee -a $target_file_name > /dev/null
    echo Exec=$script_dir/Pdc.sh --deeplink-params=%u | tee -a $target_file_name  > /dev/null
    echo Terminal=true | tee -a $target_file_name  > /dev/null
    echo Type=Application | tee -a $target_file_name  > /dev/null
    echo "Categories=Qt;Utility;" | tee -a $target_file_name  > /dev/null
    echo "MimeType=x-scheme-handler/pdc;" | tee -a $target_file_name  > /dev/null
}


create_desktop_icon $out_file_name

xdg-mime default Pdc.desktop x-scheme-handler/pdc

call_app "$@"
