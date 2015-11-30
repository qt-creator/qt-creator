import qbs

CppApplication {
    consoleApplication: true
    files: "%{CFileName}"

    Group {     // Properties for the produced executable
        fileTagsFilter: product.type
        qbs.install: true
    }
}
