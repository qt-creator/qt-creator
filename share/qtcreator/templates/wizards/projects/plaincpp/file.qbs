import qbs

CppApplication {
    consoleApplication: true
    files: "%{CppFileName}"

    Group {     // Properties for the produced executable
        fileTagsFilter: product.type
        qbs.install: true
    }
}
