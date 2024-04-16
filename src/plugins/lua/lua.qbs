import qbs 1.0

QtcPlugin {
    name: "Lua"

    Depends { name: "Core" }
    Depends { name: "Qt"; submodules: ["network", "widgets"] }

    Depends { name: "lua546" }
    Depends { name: "sol2" }

    files: [
        // "generateqtbindings.cpp", // use this if you need to generate some code
        "lua_global.h",
        "luaengine.cpp",
        "luaengine.h",
        "luaplugin.cpp",
        "luapluginloader.cpp",
        "luapluginloader.h",
        "luapluginspec.cpp",
        "luapluginspec.h",
        "luaqttypes.cpp",
        "luaqttypes.h",
        "luatr.h",
        "luauibindings.cpp",
    ]

    Group {
        name: "Bindings"
        prefix: "bindings/"

        files: [
            "action.cpp",
            "async.cpp",
            "core.cpp",
            "fetch.cpp",
            "hook.cpp",
            "inheritance.h",
            "layout.cpp",
            "messagemanager.cpp",
            "qtcprocess.cpp",
            "settings.cpp",
            "utils.cpp",
        ]

        Properties {
            condition: qbs.toolchain.contains("msvc")
            cpp.cxxFlags: "/bigobj"
        }
    }

    Group {
        name: "Meta"
        prefix: "meta/"

        files: [
            "action.lua",
            "async.lua",
            "core.lua",
            "fetch.lua",
            "layout.lua",
            "lsp.lua",
            "messagemanager.lua",
            "process.lua",
            "qt.lua",
            "qtc.lua",
            "settings.lua",
            "simpletypes.lua",
            "utils.lua",
            "widgets.lua",
            "wizard.lua",
        ]
    }

    Group {
        name: "Meta-Backup"
        prefix: "metabackup/"
        files: [
            "qobject.lua",
        ]
    }
}
