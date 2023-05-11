import qbs

QtcAutotest {
    name: "Utils::Text autotest"
    Depends { name: "Utils" }
    files: "tst_text.cpp"
}
