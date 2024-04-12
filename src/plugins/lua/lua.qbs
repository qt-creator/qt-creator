import qbs 1.0

QtcPlugin {
    name: "Lua"

    Depends { name: "Core" }
    Depends { name: "Qt"; submodules: ["widgets"] }

    // TODO: Find Lua library, or disable the plugin
    Depends { name: "Lua"; }

    //cpp.defines: LUA_AVAILABLE SOL_ALL_SAFETIES_ON=1

    files: [
        "luaplugin.cpp",
        "luaplugin.h",
        "luaengine.cpp",
        "luaengine.h",
        "luaapiregistry.cpp",
        "luaapiregistry.h",
        "luapluginloader.cpp",
        "luapluginloader.h",
        "luatr.h"
    ]
}

