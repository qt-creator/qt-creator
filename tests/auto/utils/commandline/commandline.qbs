Project  {
    QtcAutotest {
        name: "CommandLine autotest"

        Depends { name: "Utils" }
        Depends { name: "app_version_header" }

        files: [
            "tst_commandline.cpp",
        ]
    }
}
