import qbs

Project {
    name: "Valgrind autotests"
    references: [
        "callgrind/callgrind.qbs",
        "memcheck/memcheck.qbs"
    ]
}
