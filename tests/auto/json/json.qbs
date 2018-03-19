import qbs

QtcAutotest {
    name: "json autotest"
    Depends { name: "bundle" }
    Depends { name: "qtcjson" }

    consoleApplication: true

    cpp.defines: base.filter(function(d) {
        return d !== "QT_USE_FAST_OPERATOR_PLUS"
            && d !== "QT_USE_FAST_CONCATENATION";
    })

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
