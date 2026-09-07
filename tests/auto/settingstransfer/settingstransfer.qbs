import qbs

QtcAutotest {
    name: "SettingsTransfer autotest"
    Depends { name: "Core" }
    Depends { name: "Utils" }

    Group {
        name: "Test sources"
        files: "tst_settingstransfer.cpp"
    }
}
