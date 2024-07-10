QtcLibrary {
    name: "lua546"
    type: "staticlibrary"

    cpp.defines: {
        var defines = base;
        if (qbs.targetOS.contains("windows"))
            defines.push("LUA_USE_WINDOWS");
        else if (qbs.targetOS.contains("macos"))
            defines.push("LUA_USE_MACOSX");
        else if (qbs.targetOS.contains("linux"))
            defines.push("LUA_USE_LINUX");
        return defines;
    }

    Group {
        name: "Sources"
        prefix: "src/"

        files: [
            "lapi.c",
            "lapi.h",
            "lauxlib.c",
            "lauxlib.h",
            "lbaselib.c",
            "lcode.c",
            "lcode.h",
            "lcorolib.c",
            "lctype.c",
            "lctype.h",
            "ldblib.c",
            "ldebug.c",
            "ldebug.h",
            "ldo.c",
            "ldo.h",
            "ldump.c",
            "lfunc.c",
            "lfunc.h",
            "lgc.c",
            "lgc.h",
            "linit.c",
            "liolib.c",
            "llex.c",
            "llex.h",
            "lmathlib.c",
            "lmem.c",
            "lmem.h",
            "loadlib.c",
            "lobject.c",
            "lobject.h",
            "lopcodes.c",
            "lopcodes.h",
            "loslib.c",
            "lparser.c",
            "lparser.h",
            "lstate.c",
            "lstate.h",
            "lstring.c",
            "lstring.h",
            "lstrlib.c",
            "ltable.c",
            "ltable.h",
            "ltablib.c",
            "ltm.c",
            "ltm.h",
            "lua.c",
            "lua.h",
            "lundump.c",
            "lundump.h",
            "lutf8lib.c",
            "lvm.c",
            "lvm.h",
            "lzio.c",
            "lzio.h",
        ]
    }

    Export {
        cpp.includePaths: project.ide_source_tree + "/src/libs/3rdparty/lua/src"
    }
}
