import qbs

QtcAutotest {
    name: "DevContainer features autotest"
    Depends { name: "DevContainer" }
    Depends { name: "Utils" }

    files: "tst_devcontainerfeatures.cpp"
}
