#!/bin/bash
script_dir=$( dirname "$(readlink -f "$0")" )

out_dir=~/.local/share/applications
out_file_name="${out_dir}/Pdc.desktop"

export QTWEBENGINE_DISABLE_SANDBOX=1
export QTWEBENGINE_CHROMIUM_FLAGS="${QTWEBENGINE_CHROMIUM_FLAGS:---disable-gpu --no-sandbox}"
# Prefer libraries shipped inside the AppImage over host OpenSSL 3.
if [ -d "$script_dir/usr/lib" ]; then
  export LD_LIBRARY_PATH="$script_dir/usr/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
fi
if [ -x "$script_dir/usr/bin/QtWebEngineProcess" ]; then
  export QTWEBENGINEPROCESS_PATH="$script_dir/usr/bin/QtWebEngineProcess"
elif [ -x "$script_dir/QtWebEngineProcess" ]; then
  export QTWEBENGINEPROCESS_PATH="$script_dir/QtWebEngineProcess"
fi

call_app()
{
  pushd "$script_dir" >/dev/null
  usr/bin/Pdc "$@"
  status=$?
  if [ $status -ne 0 ]; then
    echo $'\n\n\x1b[1mIf Pdc fails to launch, it might need to install xinerama extension for the X C Binding with this command:\n\x1b[2m   sudo apt-get install libxcb-xinerama0\n\n'
  fi

  popd >/dev/null
  exit $status
}


create_desktop_icon()
{
    target_file_name=$1
    echo "Generating icon file: $target_file_name..."
    rm -f "${out_dir}/Pdc.png"
    rm -f $target_file_name
    cp -Rv "${APPDIR}/usr/share/icons/hicolor/256x256/apps/Pdc.png" "${out_dir}/Pdc.png"
    echo [Desktop Entry] | tee -a $target_file_name  > /dev/null
    echo Version=1.0 | tee -a $target_file_name  > /dev/null
    echo Name=Pdc | tee -a $target_file_name > /dev/null
    echo GenericName=Pdc | tee -a $target_file_name  > /dev/null
    echo Comment=Privacy blockchain | tee -a $target_file_name > /dev/null
    echo Icon=${out_dir}/Pdc.png | tee -a $target_file_name > /dev/null
    echo Exec=$APPIMAGE --deeplink-params=\\\"%u\\\" | tee -a $target_file_name  > /dev/null
    echo Terminal=false | tee -a $target_file_name  > /dev/null
    echo Type=Application | tee -a $target_file_name  > /dev/null
    echo "Categories=Qt;Utility;" | tee -a $target_file_name  > /dev/null
    echo "MimeType=x-scheme-handler/pdc;" | tee -a $target_file_name  > /dev/null
    echo "StartupWMClass=Pdc" | tee -a $target_file_name  > /dev/null
}


create_desktop_icon $out_file_name

xdg-mime default Pdc.desktop x-scheme-handler/pdc

call_app "$@"
