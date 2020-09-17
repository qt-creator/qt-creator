Project {
    name: "C++ unit tests"
    condition: project.withAutotests
    references: [
        "echoserver/echoserver.qbs",
        "unittest/unittest.qbs",
    ]
}
