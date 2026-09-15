QtcAutotest {
    name: "CMakeLang autotest"
    Depends { name: "CMakeLang" }
    Depends { name: "RstLang" }
    cpp.defines: base.concat('SRCDIR="' + path + '"')
    files: "tst_cmakelang.cpp"
}
