add_qtc_plugin(LuaLanguageClient
  CONDITION TARGET Lua
  PLUGIN_DEPENDS LanguageClient Lua
  SOURCES
    lualanguageclient.cpp
)
