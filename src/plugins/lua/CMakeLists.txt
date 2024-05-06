
add_qtc_plugin(Lua
  PLUGIN_DEPENDS Core
  PUBLIC_DEPENDS lua546 sol2
  PUBLIC_DEFINES LUA_AVAILABLE
  SOURCES
    bindings/action.cpp
    bindings/async.cpp
    bindings/core.cpp
    bindings/fetch.cpp
    bindings/hook.cpp
    bindings/inheritance.h
    bindings/layout.cpp
    bindings/messagemanager.cpp
    bindings/qtcprocess.cpp
    bindings/settings.cpp
    bindings/utils.cpp
    luaengine.cpp
    luaengine.h
    luaplugin.cpp
    luapluginspec.cpp
    luapluginspec.h
    luaqttypes.cpp
    luaqttypes.h
    luatr.h
    # generateqtbindings.cpp # Use this if you need to generate some code.
)


set_source_files_properties(luauibindings.cpp PROPERTY SKIP_AUTOMOC ON PROPERTY SKIP_AUTOGEN ON)

if (MSVC)
    # Prevent fatal error C1128
    set_property(SOURCE bindings/settings.cpp PROPERTY COMPILE_FLAGS /bigobj)
endif()
