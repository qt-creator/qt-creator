import qbs

QtcAutotest {
    name: "ElfReader autotest"
    Depends { name: "Utils" }
    files: "tst_elfreader.cpp"
}
