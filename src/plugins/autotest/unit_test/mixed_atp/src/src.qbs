import qbs

CppApplication {
    type: "application"
    name: "Dummy Application"

    Depends { name: "Qt.gui" }
    Depends { name: "Qt.widgets" }

    files: [ "main.cpp" ]
}
