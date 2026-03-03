QtcLibrary {
    name: "McpServerLib"
    Depends { name: "Utils" }
    Depends { name: "Qt"; submodules: ["core", "network"] }
    Depends { name: "Qt.httpserver"; required: false }

    cpp.defines: base.concat("MCPSERVERLIB_LIBRARY")

    Properties {
        condition: Qt.httpserver.present
        cpp.defines: outer.concat("MCP_SERVER_HAS_QT_HTTP_SERVER")
    }

    Group {
        prefix: "server/"
        files: [
            "mcpserver.cpp",
            "mcpserver.h",
            "mcpserver_global.h",
            "minihttpserver.h",
            "../schemas/schema_2025_11_25.cpp",
            "../schemas/schema_2025_11_25.h",
        ]
    }

    Properties {
        condition: qbs.toolchain.contains("msvc")
        cpp.cxxFlags: "/bigobj"
    }
    Properties {
        condition: qbs.toolchain.contains("mingw")
        cpp.cxxFlags: "-Wa,-mbig-obj"
    }
}

