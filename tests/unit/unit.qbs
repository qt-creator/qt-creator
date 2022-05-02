Project {
    name: "C++ unit tests"
    condition: project.withAutotests
    references: [
        "unittest/unittest.qbs",
    ]
}
