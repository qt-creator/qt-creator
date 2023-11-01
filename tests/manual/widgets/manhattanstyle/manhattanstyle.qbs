import "../common/common.qbs" as Common

CppApplication {
    name: "Manual Test Utils ManhattanStyle"

    Depends { name: "Core" }
    Depends { name: "Utils" }

    files: "tst_manual_widgets_manhattanstyle.cpp"

    Common {}
}
