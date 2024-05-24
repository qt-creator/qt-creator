
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
    bindings/install.cpp
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
    meta/action.lua
    meta/async.lua
    meta/core.lua
    meta/fetch.lua
    meta/install.lua
    meta/layout.lua
    meta/lsp.lua
    meta/messagemanager.lua
    meta/process.lua
    meta/qt.lua
    meta/qtc.lua
    meta/settings.lua
    meta/simpletypes.lua
    meta/utils.lua
    meta/widgets.lua
    meta/wizard.lua
    # generateqtbindings.cpp # Use this if you need to generate some code.
)


set_source_files_properties(luauibindings.cpp PROPERTY SKIP_AUTOMOC ON PROPERTY SKIP_AUTOGEN ON)

if (MSVC)
    # Prevent fatal error C1128
    set_property(SOURCE bindings/settings.cpp PROPERTY COMPILE_FLAGS /bigobj)
endif()

set(META_FILES
  meta/action.lua
  meta/async.lua
  meta/core.lua
  meta/fetch.lua
  meta/layout.lua
  meta/lsp.lua
  meta/messagemanager.lua
  meta/process.lua
  meta/qt.lua
  meta/qtc.lua
  meta/settings.lua
  meta/simpletypes.lua
  meta/utils.lua
  meta/widgets.lua
  meta/wizard.lua
)

qtc_copy_to_builddir(copy_lua_meta_files
  DESTINATION ${IDE_DATA_PATH}/lua
  FILES ${META_FILES}
)

install(
  FILES ${META_FILES}
  DESTINATION ${IDE_DATA_PATH}/lua/meta
)
