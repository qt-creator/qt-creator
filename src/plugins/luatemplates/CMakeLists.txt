

add_qtc_plugin(LuaTemplates
  CONDITION TARGET Lua
  PLUGIN_DEPENDS Lua TextEditor ProjectExplorer Core
  SOURCES
    luatemplates.cpp
)

add_subdirectory(templates)
