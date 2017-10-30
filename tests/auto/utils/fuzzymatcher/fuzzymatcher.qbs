import qbs

QtcAutotest {
    name: "FuzzyMatcher autotest"
    Depends { name: "Utils" }
    files: "tst_fuzzymatcher.cpp"
}
