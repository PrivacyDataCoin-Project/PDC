set(UPNPC_BUILD_STATIC ON CACHE BOOL "Build static library")
set(UPNPC_BUILD_SHARED OFF CACHE BOOL "Build shared library")
set(UPNPC_BUILD_TESTS OFF CACHE BOOL "Build test executables")

# Warning/security fixes live in the PrivacyDataCoin-Project submodule forks
# (see contrib/pdc-fixes/README.md). Do not file(COPY) into submodule trees —
# that dirties the working tree and makes git describe append -dirty.

add_subdirectory(zlib)
add_subdirectory(db)
add_subdirectory(ethereum)
add_subdirectory(randomx)
set_property(TARGET randomx PROPERTY FOLDER "contrib")
if(TARGET randomx-tests)
  set_property(TARGET randomx-tests randomx-codegen randomx-benchmark PROPERTY EXCLUDE_FROM_ALL TRUE)
  set_property(TARGET randomx-tests randomx-codegen randomx-benchmark PROPERTY EXCLUDE_FROM_DEFAULT_BUILD TRUE)
  set_property(TARGET randomx-tests randomx-codegen randomx-benchmark PROPERTY FOLDER "unused")
endif()

if(MSVC)
  # Keep these C/C++ disables off ml64. MASM reports A4018 for /wd.
  foreach(_pdc_rx_wd /wd4067 /wd4101 /wd4244 /wd4267 /wd4996)
    target_compile_options(randomx PRIVATE
      $<$<COMPILE_LANGUAGE:C>:${_pdc_rx_wd}>
      $<$<COMPILE_LANGUAGE:CXX>:${_pdc_rx_wd}>)
  endforeach()
  unset(_pdc_rx_wd)
elseif(CMAKE_C_COMPILER_ID MATCHES "Clang")
  # Language-scoped options: C-only flags must not be passed to the C++ compiler.
  target_compile_options(randomx PRIVATE
    $<$<COMPILE_LANGUAGE:C>:-Wno-strict-prototypes>
    -Wno-unused-function -Wno-sign-compare -Wno-type-limits
    -Wno-ignored-qualifiers -Wno-implicit-fallthrough -Wno-unknown-warning-option)
else()
  target_compile_options(randomx PRIVATE
    $<$<COMPILE_LANGUAGE:C>:-Wno-strict-prototypes -Wno-discarded-qualifiers>
    -Wno-unused-function -Wno-unused-but-set-variable -Wno-sign-compare
    -Wno-type-limits -Wno-implicit-fallthrough)
endif()

if(USE_BITCOIN_SECP256K1_FOR_ECDSA)
  option(SECP256K1_BUILD_BENCHMARK "Build benchmarks." OFF)
  option(SECP256K1_BUILD_TESTS "Build tests." OFF)
  option(SECP256K1_BUILD_EXHAUSTIVE_TESTS "Build exhaustive tests." OFF)
  option(SECP256K1_BUILD_CTIME_TESTS "Build constant-time tests." OFF)
  option(SECP256K1_BUILD_EXAMPLES "Build examples." OFF)
  set_property(GLOBAL PROPERTY CTEST_TARGETS_ADDED 1)

  if(STATIC)
    set(SECP256K1_DISABLE_SHARED ON CACHE BOOL "Disable shared library for secp256k1")
    set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build static libraries by default" FORCE) 
  endif()
  add_subdirectory(bitcoin-secp256k1)
  set_property(TARGET secp256k1 PROPERTY FOLDER "contrib")
  set_property(TARGET secp256k1_precomputed PROPERTY FOLDER "contrib")
endif()

if( NOT DISABLE_TOR)
  add_subdirectory(tor-connect)
endif()


if(CMAKE_SYSTEM_NAME STREQUAL "iOS" OR CMAKE_SYSTEM_NAME STREQUAL "Android")
  message("excluded upnp support for IOS build")
  return()
endif()

add_subdirectory(miniupnp/miniupnpc)



set_property(TARGET libminiupnpc-static PROPERTY FOLDER "contrib")
set_property(TARGET zlibstatic PROPERTY FOLDER "contrib")
set_property(TARGET mdbx PROPERTY FOLDER "contrib")
set_property(TARGET lmdb PROPERTY FOLDER "contrib")

if( NOT DISABLE_TOR)
  set_property(TARGET tor-connect PROPERTY FOLDER "contrib")
endif()

set_property(TARGET upnpc-static mdbx_chk mdbx_copy mdbx_dump mdbx_load mdbx_stat PROPERTY FOLDER "unused")

if(MSVC)
  #set_property(TARGET ntdll_extra_target PROPERTY FOLDER "unused")
endif()


if(CMAKE_C_COMPILER_ID MATCHES "Clang")
  target_compile_options(libminiupnpc-static PRIVATE
    -Wno-undef -Wno-unused-result -Wno-unused-value -Wno-implicit-fallthrough
    -Wno-ignored-qualifiers -Wno-unknown-warning-option)
  # zlib 1.3.1 uses ANSI prototypes; keep only residual noise filters if any remain.
  target_compile_options(zlibstatic PRIVATE
    -Wno-undef -Wno-unused-result -Wno-unused-value -Wno-implicit-fallthrough
    -Wno-ignored-qualifiers -Wno-unknown-warning-option)
elseif(NOT MSVC)
  target_compile_options(libminiupnpc-static PRIVATE
    -Wno-undef -Wno-unused-result -Wno-unused-value -Wno-implicit-fallthrough)
  target_compile_options(zlibstatic PRIVATE
    -Wno-undef -Wno-unused-result -Wno-unused-value -Wno-implicit-fallthrough)
endif()
