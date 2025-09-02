import "../common/common.qbs" as Common

CppApplication {
    name: "Manual Test Utils LayoutBuilder"

    Depends { name: "Core" }
    Depends { name: "Utils" }

    files: "tst_manual_widgets_layoutbuilder.cpp"

    Common {}
}
