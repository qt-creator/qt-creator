Project  {
    QtcAutotest {
        name: "Archive autotest"

        Depends { name: "Utils" }
        Depends { name: "karchive" }

        files: [
            "tst_archive.cpp",
        ]
    }
}
