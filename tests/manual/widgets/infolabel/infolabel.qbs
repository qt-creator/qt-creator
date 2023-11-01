import "../common/common.qbs" as Common

CppApplication {
    name: "Manual Test Utils InfoLabel"

    Depends { name: "Core" }
    Depends { name: "Utils" }

    files: "tst_manual_widgets_infolabel.cpp"

    Common {}
}
