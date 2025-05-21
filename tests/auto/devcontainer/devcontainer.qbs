import qbs

QtcAutotest {
    name: "DevContainer autotest"
    Depends { name: "DevContainer" }
    files: "tst_devcontainer.cpp"
}
