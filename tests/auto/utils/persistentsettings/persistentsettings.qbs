import qbs

QtcAutotest {
    name: "PersistentSettings autotest"
    Depends { name: "Utils" }
    files: "tst_persistentsettings.cpp"
}
