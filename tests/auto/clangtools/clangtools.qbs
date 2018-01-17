import qbs

Project {
    name: "ClangTools autotests"
    references: [
        "clangtoolslogfilereader",
        "clangstaticanalyzerrunner",
    ]
}
