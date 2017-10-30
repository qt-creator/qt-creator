import qbs

QtcAutotest {
    name: "CamelHumpMatcher autotest"
    Depends { name: "Utils" }
    files: "tst_camelhumpmatcher.cpp"
}
