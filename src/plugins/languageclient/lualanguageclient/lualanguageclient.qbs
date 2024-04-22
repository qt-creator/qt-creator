QtcPlugin {
    name: "LuaLanguageClient"

    Depends { name: "Core" }
    Depends { name: "LanguageClient" }
    Depends { name: "Lua" }
    Depends { name: "ProjectExplorer" }
    Depends { name: "TextEditor" }
    Depends { name: "Utils" }

    files: [
        "lualanguageclient.cpp",
    ]
}
