import qbs

QtcAutotest {
    name: "Markdown autotest"
    Depends { name: "Utils" }
    files: "tst_markdown.cpp"
}
