import qbs

QtcAutotest {
    name: "Environment autotest"
    Depends { name: "Utils" }
    Depends { name: "Qt.widgets" } // TODO: qbs bug, remove when fixed
    files: "tst_environment.cpp"
}
