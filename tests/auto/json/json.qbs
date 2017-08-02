import qbs

QtcAutotest {
    name: "json autotest"
    Depends { name: "bundle" }
    Depends { name: "qtcjson" }

    consoleApplication: true

    Group {
        name: "test data"
        files: [
            "bom.json",
            "test.bjson",
            "test.json",
            "test2.json",
            "test3.json",
        ]
    }

    files: ["tst_json.cpp"]
}
