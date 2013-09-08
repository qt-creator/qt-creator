import qbs

Project {
    name: "Valgrind autotests"
    condition: qbs.targetOS.contains("unix") // FIXME: doesn't link on Windows
    references: [
        "callgrind/callgrind.qbs",
        "memcheck/memcheck.qbs"
    ]
}
