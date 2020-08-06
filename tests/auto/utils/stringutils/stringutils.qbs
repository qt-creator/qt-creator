import qbs

QtcAutotest {
    name: "StringUtils autotest"
    Depends { name: "Utils" }
    cpp.defines: base.filter(function(d) {
        return d !== "QT_USE_QSTRINGBUILDER";
    })
    files: "tst_stringutils.cpp"
}
