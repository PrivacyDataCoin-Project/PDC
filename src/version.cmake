# Resolve BUILD_COMMIT_ID for src/version.h.in (@VERSION@).
# Prefer a short hash from `git describe --match vX.Y.Z`; if that fails (no
# matching tag) or the regex yields nothing, fall back to rev-parse so the
# version string never ends up as "3.1.0.12[]".

execute_process(
  COMMAND "${GIT}" describe --dirty --match "v${VERSION}"
  RESULT_VARIABLE RET
  OUTPUT_VARIABLE DESCRIPTION
  OUTPUT_STRIP_TRAILING_WHITESPACE
  ERROR_QUIET
)

set(_commit "")
if(NOT RET)
  string(REGEX MATCH "[^0-9a-f]([0-9a-f][0-9a-f][0-9a-f][0-9a-f][0-9a-f][0-9a-f][0-9a-f])[0-9a-f]*(-dirty)?" STUB_VAR "${DESCRIPTION}")
  set(_commit "${CMAKE_MATCH_1}${CMAKE_MATCH_2}")
endif()

if(_commit STREQUAL "")
  if(RET)
    message(WARNING "Cannot determine current revision via git describe (match v${VERSION}). Falling back to rev-parse.")
  else()
    message(WARNING "git describe produced no commit id ('${DESCRIPTION}'). Falling back to rev-parse.")
  endif()
  execute_process(
    COMMAND "${GIT}" rev-parse --short=7 HEAD
    RESULT_VARIABLE REV_RET
    OUTPUT_VARIABLE _commit
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
  )
  if(REV_RET OR _commit STREQUAL "")
    if(DEFINED COMMIT AND NOT COMMIT STREQUAL "")
      set(_commit "${COMMIT}")
    else()
      set(_commit "unknown")
    endif()
  endif()
endif()

set(VERSION "${_commit}")
configure_file("src/version.h.in" "${TO}")
