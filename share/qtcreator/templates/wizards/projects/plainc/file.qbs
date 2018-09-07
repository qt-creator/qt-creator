import qbs

CppApplication {
    consoleApplication: true
    files: "%{CFileName}"

    Group {     // Properties for the produced executable
        fileTagsFilter: "application"
        qbs.install: true
        qbs.installDir: "bin"
    }
}
