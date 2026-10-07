
set (lmdb_sources mdb.c midl.c)

include_directories("${CMAKE_CURRENT_SOURCE_DIR}")

if(NOT MSVC)
  if(CMAKE_C_COMPILER_ID MATCHES "Clang")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-missing-field-initializers -Wno-missing-braces -Wno-aggregate-return -Wno-ignored-qualifiers -Wno-implicit-fallthrough -Wno-unknown-warning-option ")
  else()
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-missing-field-initializers -Wno-missing-braces -Wno-aggregate-return -Wno-discarded-qualifiers -Wno-unused-but-set-variable -Wno-implicit-fallthrough -Wno-maybe-uninitialized ")
  endif()
endif()
if(FREEBSD)
  add_definitions(-DMDB_DSYNC=O_SYNC)
endif()

add_library(lmdb  ${lmdb_sources})

target_link_libraries(lmdb PRIVATE ${CMAKE_THREAD_LIBS_INIT})

if(WIN32)
  target_link_libraries(lmdb PRIVATE ntdll)
endif()
