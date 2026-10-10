## Cloning

Be sure to clone the repository properly:\
`$ git clone --recursive https://github.com/PrivacyDataCoin-Project/PDC.git`

# Building
--------


### Dependencies

Versions below are the ones used by the current release CI (the Linux AppImage, Windows, and macOS jobs on `master`). A local build that should match a release binary should use these, not the older Qt 5 / OpenSSL 1.1 pins.

| component | release CI | notes |
|--|--|--|
| gcc (Linux) | 11 (Ubuntu 22.04) | x64 release images are built on Ubuntu 22.04 |
| [MSVC](https://visualstudio.microsoft.com/downloads/) | 2022 (windows-2022) | |
| [Xcode](https://developer.apple.com/downloads/) | macOS 14+ host | GUI deployment target is macOS 12.0 because of Qt 6.8 |
| [CMake](https://cmake.org/download/) | 3.16 or newer | `CMakeLists.txt` requires 3.16 |
| [Boost](https://www.boost.org/users/download/) | **1.84.0** | static libraries, `runtime-link=static` when `STATIC=TRUE`. Components: system, filesystem, thread, date_time, chrono, regex, serialization, atomic, program_options, locale, timer, log |
| [OpenSSL](https://www.openssl.org/source/) | **3.5.8** on Linux and Windows, **3.5.7** on macOS | static (`no-shared`) in release CI |
| [Qt](https://download.qt.io/archive/qt/6.8/6.8.3/) (*GUI only*) | **6.8.3** | modules: Widgets, WebEngine, WebChannel, PrintSupport, plus WebEngine's `qtpositioning` and `qtserialport`. CMake still accepts Qt 5 if `Qt5WebEngineWidgets` is found; release builds do not use it |

Note:\
[*server version*] denotes steps required for building command-line tools (`pdcd`, `simplewallet`, `connectivity_tool`).\
[*GUI version*] denotes steps required for building the `Pdc` executable.

<br />

### Linux

Recommended OS for a build that matches the release AppImage: Ubuntu 22.04 LTS. Newer hosts can compile too; the shipped x64 image is still built on 22.04.

1. Packages

   [*server version*]

       sudo apt-get update
       sudo apt-get install -y --no-install-recommends \
         build-essential g++ cmake git curl ca-certificates bzip2 pkg-config perl \
         libbz2-dev zlib1g-dev libicu-dev

   [*GUI version*] — the server packages, plus the libraries the Linux CI image installs before configuring Qt 6.8:

       sudo apt-get update
       sudo apt-get install -y --no-install-recommends \
         build-essential g++ cmake git curl ca-certificates bzip2 pkg-config perl \
         python3 python3-pip python3-venv \
         libbz2-dev zlib1g-dev libicu-dev \
         libevent-dev libminizip-dev \
         libgl1-mesa-dev libglu1-mesa-dev \
         libnss3 libnspr4 \
         libxkbcommon-x11-0 libxcb-cursor0 libxcb-xinerama0 \
         desktop-file-utils file patchelf rsync

   `python3` is only needed if you install Qt with `aqtinstall` (step 4). `patchelf` is only needed when packing an AppImage, not for a plain `cmake` build.

2. Clone PDC into a local folder\
   (The default branch is master. To use another branch, add `-b` and the branch name.)
   
       git clone --recursive https://github.com/PrivacyDataCoin-Project/PDC.git

   In the following steps we assume that you cloned PDC into '~/pdc' folder in your home directory. 

3. Download and build Boost 1.84.0

   Release CI builds static Boost 1.84.0 with `runtime-link=static`, which `STATIC=TRUE` requires. The `log` component is required (`find_package` asks for it). Checksums match `.github/workflows/build-linux.yml`.

       curl -fL -O https://archives.boost.io/release/1.84.0/source/boost_1_84_0.tar.bz2
       echo "cc4b893acf645c9d4b698e9a0f08ca8846aa5d6c68275c14c3e7949c24109454  boost_1_84_0.tar.bz2" | sha256sum -c
       tar -xjf boost_1_84_0.tar.bz2
       cd boost_1_84_0
       ./bootstrap.sh --prefix=$HOME/boost_1_84_0 \
         --with-libraries=system,filesystem,thread,date_time,chrono,regex,serialization,atomic,program_options,locale,timer,log
       ./b2 -j"$(nproc)" link=static runtime-link=static threading=multi variant=release install
       cd ..

   Installed tree: `$HOME/boost_1_84_0` (headers and `lib/libboost_*.a`).

4. Install Qt 6.8.3\
(*GUI version only*)

   Release CI installs Qt **6.8.3** for the desktop kit, with modules `qtwebengine`, `qtwebchannel`, `qtpositioning`, and `qtserialport`. Widgets and PrintSupport come with the base kit.

       python3 -m pip install --user 'aqtinstall==3.3.0'
       python3 -m aqt install-qt linux desktop 6.8.3 linux_gcc_64 -O "$HOME/Qt" \
         -m qtwebengine qtwebchannel qtpositioning qtserialport

   Prefix used below: `$HOME/Qt/6.8.3/gcc_64`.

   The Qt online installer works too: select Qt 6.8.3, the desktop gcc 64-bit kit, and Qt WebEngine.

5. Install OpenSSL 3.5.8

   Release CI builds OpenSSL **3.5.8** static (`no-shared`) on Linux and Windows. macOS release CI uses **3.5.7**. Install it locally; do not point the build at OpenSSL 1.1.1.

       curl -fL -o openssl-3.5.8.tar.gz \
         https://github.com/openssl/openssl/releases/download/openssl-3.5.8/openssl-3.5.8.tar.gz
       echo "a8f84a39918ec6415ce765d9b429d313ba97b8143169c172e734b9514464f5b2  openssl-3.5.8.tar.gz" | sha256sum -c
       tar -xzf openssl-3.5.8.tar.gz
       cd openssl-3.5.8
       ./Configure linux-x86_64 no-shared no-tests --prefix=$HOME/openssl --openssldir=$HOME/openssl --libdir=lib
       make -j"$(nproc)" && make install_sw
       cd ..


6. [*OPTIONAL*] Set global environment variables for convenient use\
For instance, by adding the following lines to `~/.bashrc`

    [*server version*]

       export BOOST_ROOT=$HOME/boost_1_84_0
       export BOOST_LIBRARYDIR=$HOME/boost_1_84_0/lib
       export OPENSSL_ROOT_DIR=$HOME/openssl


    [*GUI version*]

       export BOOST_ROOT=$HOME/boost_1_84_0
       export BOOST_LIBRARYDIR=$HOME/boost_1_84_0/lib
       export OPENSSL_ROOT_DIR=$HOME/openssl
       export QT_PREFIX_PATH=$HOME/Qt/6.8.3/gcc_64

      **NOTICE: Please edit the lines above according to your actual paths.**
   
      **NOTICE 2:** Make sure you've restarted your terminal session (by reopening the terminal window or reconnecting the server) to apply these changes.

7. Build the binaries

   From the repository root. `STATIC=TRUE` matches the release CI link and needs the static Boost and OpenSSL trees from the steps above.

   [*server version*]

       cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DSTATIC=TRUE -DBUILD_GUI=FALSE \
         -DBOOST_ROOT="$BOOST_ROOT" -DBOOST_LIBRARYDIR="$BOOST_LIBRARYDIR" \
         -DOPENSSL_ROOT_DIR="$OPENSSL_ROOT_DIR" -DOPENSSL_USE_STATIC_LIBS=TRUE
       cmake --build build --parallel --target daemon simplewallet connectivity_tool

   Binaries: `build/src/pdcd`, `build/src/simplewallet`, `build/src/connectivity_tool`.

   [*GUI version*]

       cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DSTATIC=TRUE -DBUILD_GUI=TRUE \
         -DBOOST_ROOT="$BOOST_ROOT" -DBOOST_LIBRARYDIR="$BOOST_LIBRARYDIR" \
         -DOPENSSL_ROOT_DIR="$OPENSSL_ROOT_DIR" -DOPENSSL_USE_STATIC_LIBS=TRUE \
         -DCMAKE_PREFIX_PATH="$QT_PREFIX_PATH"
       cmake --build build --parallel --target Pdc

   Binary: `build/src/Pdc`. The GUI must link the shared `libstdc++` that Qt loads. Do not pass `-static-libstdc++` to `Pdc`; `STATIC=TRUE` applies that flag only to the CLI targets.

   For a testnet build add `-DTESTNET=TRUE`.

   `utils/build_script_linux.sh` is an older helper. Its Qt 5 library copy list does not match Qt 6.8.3; prefer the `cmake` commands above.

### Running the Linux AppImage

The release AppImage is built on Ubuntu 22.04 and is meant to run on that glibc and newer. It does not bundle GL, EGL, or X11. If the GUI fails to start, install the host libraries named by `utils/Pdc_appimage_wrapper.sh`:

    sudo apt-get install -y libegl1 libgl1 libopengl0 libxcb-xinerama0 libx11-6 libxcb1

On a machine without FUSE: `APPIMAGE_EXTRACT_AND_RUN=1 ./pdc-linux-x64-gui-*.AppImage`.

<br />

### Windows
Recommended OS version: Windows 11 x64. Release CI runs on `windows-2022` with MSVC 2022.
1. Install the same dependency versions as Linux, with the Windows CI pins: Boost **1.84.0** (static libs, shared CRT), OpenSSL **3.5.8** (static libs, shared CRT), Qt **6.8.3** (MSVC 2022 x64, WebEngine), CMake 3.16 or newer.
2. Edit paths in `utils/configure_local_paths.cmd`.
3. Run one of `utils/configure_win64_msvsNNNN_gui.cmd` according to your MSVC version.
4. Go to the build folder and open generated Pdc.sln in MSVC.
5. Build.

In order to correctly deploy Qt GUI application, you also need to do the following:

6. Copy Pdc.exe to a folder (e.g. `depoy`). 
7. Run  `PATH_TO_QT\bin\windeployqt.exe deploy\Pdc.exe`.
8. Copy folder `\src\gui\qt-daemon\html` to `deploy\html`.
9. Now you can run `Pdc.exe`

<br />

### macOS
Release CI uses Boost **1.84.0**, OpenSSL **3.5.7**, and Qt **6.8.3** (WebEngine). The GUI deployment target is macOS 12.0.
1. Install those prerequisites.
2. Set environment variables as stated in `utils/macosx_build_config.command`.
3.  `mkdir build` <br> `cd build` <br> `cmake ..` <br> `make`

To build GUI application:

1. Create self-signing certificate via Keychain Access:\
    a. Run Keychain Access.\
    b. Choose Keychain Access > Certificate Assistant > Create a Certificate.\
    c. Use “Pdc” (without quotes) as certificate name.\
    d. Choose “Code Signing” in “Certificate Type” field.\
    e. Press “Create”, then “Done”.\
    f. Make sure the certificate was added to keychain "System". If not—move it to "System".\
    g. Double click the certificate you've just added, enter the trust section and under "When using this certificate" select "Always trust".\
    h. Unfold the certificate in Keychain Access window and double click the underlying private key "Pdc". Select "Access Control" tab, then select "Allow all applications to access this item". Click "Save Changes".
2. Revise building script, comment out unwanted steps and run it:  `utils/build_script_mac_osx.sh`
3. The application should be here: `/buid_mac_osx_64/release/src`

<br />
<br />

## Supporting project/donations

PDC: @arqtras<br />
XMR: 44xwrxeWXiCQJhvxxa5sVn6etiCoRAzHkMRJP6re2ySXXCJd4S5qvshjnyhePGFSftCjBHNKAdH5e2nZyrJgNTowBdGzRaU<br />

