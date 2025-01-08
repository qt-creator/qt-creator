add_qtc_plugin(Lua
  PLUGIN_DEPENDS Core
  PUBLIC_DEPENDS lua546 sol2 TextEditor ProjectExplorer
  PUBLIC_DEFINES LUA_AVAILABLE
  SOURCES
    bindings/action.cpp
    bindings/async.h
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
    bindings/project.cpp
    bindings/qt.cpp
    bindings/qtcprocess.cpp
    bindings/settings.cpp
    bindings/taskhub.cpp
    bindings/texteditor.cpp
    bindings/translate.cpp
    bindings/utils.cpp
    bindings/utils.h
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
  SBOM_ARGS
    USE_ATTRIBUTION_FILES
    CREATE_SBOM_FOR_EACH_ATTRIBUTION
    ATTRIBUTION_FILE_PATHS
      "${QtCreator_SOURCE_DIR}/qt_attributions.json"
    ATTRIBUTION_IDS
      sol2
      ms-jpq-lua-async-await
      kikito/inspect.lua
)

if(TARGET Lua)

    qt_add_resources(Lua lua_images_rcc
      PREFIX "/lua"
      FILES
        images/settingscategory_lua.png
        images/settingscategory_lua@2x.png
    )

    qt_add_resources(Lua lua_script_rcc
      PREFIX "/lua"
      FILES
        scripts/async.lua
        scripts/ilua.lua
        scripts/inspect.lua
    )

    set_source_files_properties(luauibindings.cpp PROPERTY SKIP_AUTOMOC ON PROPERTY SKIP_AUTOGEN ON)

    if (MSVC)
        # Prevent fatal error C1128
        target_compile_options(Lua PRIVATE /bigobj)
    endif()
    if (MINGW)
        target_compile_options(Lua PRIVATE "-Wa,-mbig-obj")
    endif()

    qtc_copy_to_builddir(copy_lua_meta_files
      DIRECTORIES meta
      DESTINATION ${IDE_DATA_PATH}/lua/meta
    )
endif()
