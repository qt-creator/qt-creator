import qbs

QtcAutotest {
    name: "StringUtils autotest"
    Depends { name: "Utils" }
    cpp.defines: base.filter(function(d) {
        return d !== "QT_USE_FAST_OPERATOR_PLUS"
            && d !== "QT_USE_FAST_CONCATENATION";
    })
    files: "tst_stringutils.cpp"
}
