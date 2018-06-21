import qbs

QtcAutotest {
    Depends { name: "Tracing" }
    Depends { name: "Qt.quick" }
    Depends { name: "Qt.gui" }
}
