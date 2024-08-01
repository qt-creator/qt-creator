add_qtc_plugin(Lua
  PLUGIN_DEPENDS Core
  PUBLIC_DEPENDS lua546 sol2 TextEditor
  PUBLIC_DEFINES LUA_AVAILABLE
  SOURCES
    bindings/action.cpp
    bindings/async.cpp
    bindings/core.cpp
    bindings/fetch.cpp
    bindings/gui.cpp
    bindings/hook.cpp
    bindings/inheritance.h
    bindings/install.cpp
    bindings/json.cpp
    bindings/localsocket.cpp
    bindings/macro.cpp
    bindings/messagemanager.cpp
    bindings/qt.cpp
    bindings/qtcprocess.cpp
    bindings/settings.cpp
    bindings/texteditor.cpp
    bindings/translate.cpp
    bindings/utils.cpp
    luaengine.cpp
    luaengine.h
    luaexpander.cpp
    luaplugin.cpp
    luapluginspec.cpp
    luapluginspec.h
    luaqttypes.cpp
    luaqttypes.h
    luatr.h
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

qtc_copy_to_builddir(copy_lua_meta_files
  DIRECTORIES meta
  DESTINATION ${IDE_DATA_PATH}/lua/meta
)
