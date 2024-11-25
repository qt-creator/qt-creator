import "../common/common.qbs" as Common

CppApplication {
    name: "Manual Test Utils StyledBar"

    Depends { name: "Core" }
    Depends { name: "Utils" }

    files: "tst_manual_widgets_styledbar.cpp"

    Common {}
}
