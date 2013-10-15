import qbs

Project {
    name: "Valgrind autotests"
    condition: !qbs.targetOS.contains("windows")
    references: [
        "callgrind/callgrind.qbs",
        "memcheck/memcheck.qbs"
    ]
}
