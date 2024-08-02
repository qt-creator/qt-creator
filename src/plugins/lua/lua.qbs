import qbs 1.0

QtcPlugin {
    name: "Lua"

    Depends { name: "Core" }
    Depends { name: "Qt"; submodules: ["network", "widgets"] }

    Depends { name: "lua546" }
    Depends { name: "sol2" }
    Depends { name: "TextEditor" }

    Properties {
        condition: qbs.toolchain.contains("mingw")
        cpp.optimization: "fast"
    }

    files: [
        // "generateqtbindings.cpp", // use this if you need to generate some code
        "lua_global.h",
        "luaengine.cpp",
        "luaengine.h",
        "luaexpander.cpp",
        "luaplugin.cpp",
        "luapluginspec.cpp",
        "luapluginspec.h",
        "luaqttypes.cpp",
        "luaqttypes.h",
        "luatr.h",
        "luauibindings.cpp",
        "wizards/wizards.qrc",
    ]

    Group {
        name: "Bindings"
        prefix: "bindings/"

        files: [
            "action.cpp",
            "async.cpp",
            "core.cpp",
            "fetch.cpp",
            "gui.cpp",
            "hook.cpp",
            "inheritance.h",
            "install.cpp",
            "json.cpp",
            "localsocket.cpp",
            "macro.cpp",
            "messagemanager.cpp",
            "qt.cpp",
            "qtcprocess.cpp",
            "settings.cpp",
            "texteditor.cpp",
            "translate.cpp",
            "utils.cpp",
        ]

        Properties {
            condition: qbs.toolchain.contains("msvc")
            cpp.cxxFlags: "/bigobj"
        }

        Properties {
            condition: qbs.toolchain.contains("mingw")
            cpp.cxxFlags: "-Wa,-mbig-obj"
        }
    }

    Group {
        name: "Meta"
        prefix: "meta/"
        files: "*.lua"
        qbs.install: true
        qbs.installDir: qtc.ide_data_path + "/lua/meta/"
    }

    Group {
        name: "Meta-Backup"
        prefix: "metabackup/"
        files: [
            "qobject.lua",
        ]
    }

    Group {
        name: "Lua scripts rcc"
        Qt.core.resourcePrefix: "lua/scripts/"
        fileTags: "qt.core.resource_data"
        files: "scripts/**"
    }

    Group {
        name: "Lua images rcc"
        Qt.core.resourcePrefix: "lua/images/"
        fileTags: "qt.core.resource_data"
        files: "images/**"
    }

    Export {
        Depends { name: "sol2" }
        Depends { name: "lua546" }
    }
}
