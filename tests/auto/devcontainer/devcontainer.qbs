import qbs

QtcAutotest {
    name: "DevContainer autotest"
    Depends { name: "DevContainer" }
    Depends { name: "Utils" }
    Depends { name: "Qt.gui" }

    files: "tst_devcontainer.cpp"
}
