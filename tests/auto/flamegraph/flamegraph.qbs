import qbs

QtcAutotest {
    name: "FlameGraph autotest"
    Depends { name: "FlameGraph" }
    Depends { name: "Qt.quick" }
    Depends { name: "Qt.gui" }
    files: "tst_flamegraph.cpp"
}
