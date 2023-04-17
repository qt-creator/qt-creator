import qbs

QtcAutotest {
    name: "Examples autotest"
    Depends { name: "Core" }
    Depends { name: "QtSupport" }
    Depends { name: "Utils" }
    files: "tst_examples.cpp"
}
