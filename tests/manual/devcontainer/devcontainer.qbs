import qbs

QtcAutotest {
    name: "DevContainer autotest"
    Depends { name: "DevContainer" }
    Depends { name: "Utils" }
    Depends { name: "Qt.gui" }
    Depends { name: "Qt.network" }

    files: "tst_devcontainer.cpp"
}
