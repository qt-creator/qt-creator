Project  {
    QtcAutotest {
        name: "DeviceShell autotest"

        Depends { name: "Utils" }
        Depends { name: "app_version_header" }

        files: [
            "tst_deviceshell.cpp",
        ]
    }
}
