import qbs

QtcAutotest {
    name: "TemplateEngine autotest"
    Depends { name: "Utils" }
    files: "tst_templateengine.cpp"
}
