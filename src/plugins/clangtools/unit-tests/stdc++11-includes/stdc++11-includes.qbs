import qbs

CppApplication {
    name : "Standard C++ Includes"
    consoleApplication: true
    cpp.cxxLanguageVersion: "c++11"

    files: "main.cpp"

    Group {     // Properties for the produced executable
        fileTagsFilter: product.type
        qbs.install: true
    }
}
