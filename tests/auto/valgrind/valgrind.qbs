import qbs

Project {
    name: "Valgrind autotests"
    condition: qbs.targetOS.contains("unix")
    references: [
        "callgrind/callgrind.qbs",
        "memcheck/memcheck.qbs"
    ]
}
