Project  {
    QtcAutotest {
        name: "TerminalInterface autotest"

        Depends { name: "Utils" }
        Depends { name: "app_version_header" }
        Depends { name: "Qt.network" }

        files: [
            "tst_terminalinterface.cpp",
        ]
    }
}
