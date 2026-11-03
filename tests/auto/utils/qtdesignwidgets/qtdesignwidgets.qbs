import qbs

QtcAutotest {
    name: "QtDesignWidgets autotest"
    Depends { name: "Utils" }
    files: "tst_qtdesignwidgets.cpp"
}
