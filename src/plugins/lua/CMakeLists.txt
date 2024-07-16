
add_qtc_plugin(Lua
  PLUGIN_DEPENDS Core
  PUBLIC_DEPENDS lua546 sol2 TextEditor
  PUBLIC_DEFINES LUA_AVAILABLE
  SOURCES
    bindings/action.cpp
    bindings/async.cpp
    bindings/core.cpp
    bindings/documents.cpp
    bindings/documents.h
    bindings/fetch.cpp
    bindings/gui.cpp
    bindings/hook.cpp
    bindings/inheritance.h
    bindings/install.cpp
    bindings/json.cpp
    bindings/localsocket.cpp
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
    meta/documents.lua
    meta/fetch.lua
    meta/gui.lua
    meta/json.lua
    meta/install.lua
    meta/localsocket.lua
    meta/lsp.lua
    meta/messagemanager.lua
    meta/process.lua
    meta/qt.lua
    meta/qtc.lua
    meta/settings.lua
    meta/simpletypes.lua
    meta/utils.lua
    meta/widgets.lua
    wizards/wizards.qrc
    # generateqtbindings.cpp # Use this if you need to generate some code.
)

qt_add_resources(Lua lua_images_rcc
  PREFIX "/lua"
  FILES
    images/settingscategory_lua.png
    images/settingscategory_lua@2x.png
)

qt_add_resources(Lua lua_script_rcc
  PREFIX "/lua"
  FILES
    scripts/ilua.lua
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
  meta/documents.lua
  meta/fetch.lua
  meta/gui.lua
  meta/json.lua
  meta/localsocket.lua
  meta/lsp.lua
  meta/messagemanager.lua
  meta/process.lua
  meta/qt.lua
  meta/qtc.lua
  meta/settings.lua
  meta/simpletypes.lua
  meta/utils.lua
  meta/widgets.lua
)

qtc_copy_to_builddir(copy_lua_meta_files
  DESTINATION ${IDE_DATA_PATH}/lua
  FILES ${META_FILES}
)

install(
  FILES ${META_FILES}
  DESTINATION ${IDE_DATA_PATH}/lua/meta
)
