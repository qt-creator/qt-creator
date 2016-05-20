import qbs

CppApplication {
    type: "application"
    name: "Dummy application"

    Depends { name: "Qt.core" }

    files: [ "main.cpp" ]
}
