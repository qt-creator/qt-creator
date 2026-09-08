import qbs

Project {
    QtcAutotest {
        name: "MiniHttpServer autotest"
        Depends { name: "Qt.network" }
        cpp.includePaths: base.concat(project.ide_source_tree + "/src/libs")
        files: [
            "tst_minihttpserver.cpp",
            project.ide_source_tree + "/src/libs/mcp/server/minihttpserver.h",
        ]
    }

    QtcAutotest {
        name: "McpAuth autotest"
        Depends { name: "McpServerLib" }
        Depends { name: "Utils" }
        Depends { name: "Qt.network" }
        files: "tst_mcpauth.cpp"
    }

    QtcAutotest {
        name: "ToolValidation autotest"
        Depends { name: "McpServerLib" }
        Depends { name: "Utils" }
        files: "tst_toolvalidation.cpp"
    }
}
