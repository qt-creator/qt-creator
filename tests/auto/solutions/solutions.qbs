import qbs

Project {
    name: "Solutions autotests"
    references: [
        "concurrentcall/concurrentcall.qbs",
        "qprocesstask/qprocesstask.qbs",
        "tasking/tasking.qbs",
    ]
}
