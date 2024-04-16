
add_qtc_library(lua546
  PUBLIC_INCLUDES src
  STATIC
  SOURCES
    src/lapi.c
    src/lauxlib.c
    src/lbaselib.c
    src/lcode.c
    src/lcorolib.c
    src/lctype.c
    src/ldblib.c
    src/ldebug.c
    src/ldo.c
    src/ldump.c
    src/lfunc.c
    src/lgc.c
    src/linit.c
    src/liolib.c
    src/llex.c
    src/lmathlib.c
    src/lmem.c
    src/loadlib.c
    src/lobject.c
    src/lopcodes.c
    src/loslib.c
    src/lparser.c
    src/lstate.c
    src/lstring.c
    src/lstrlib.c
    src/ltable.c
    src/ltablib.c
    src/ltm.c
    src/lua.c
    src/lundump.c
    src/lutf8lib.c
    src/lvm.c
    src/lzio.c
)

extend_qtc_library(lua546
  CONDITION LINUX
  PUBLIC_DEFINES LUA_USE_LINUX
)
extend_qtc_library(lua546
  CONDITION WIN32
  PUBLIC_DEFINES LUA_USE_WINDOWS
)
extend_qtc_library(lua546
  CONDITION APPLE
  PUBLIC_DEFINES LUA_USE_MACOSX
)
