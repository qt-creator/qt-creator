Project  {
    QtcAutotest {
        name: "TerminalInterface autotest"

        Depends { name: "Utils" }
        Depends { name: "app_version_header" }

        files: [
            "tst_terminalinterface.cpp",
        ]
    }
}
