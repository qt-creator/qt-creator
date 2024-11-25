import "../common/common.qbs" as Common

CppApplication {
    name: "Manual Test Core Components"

    Depends { name: "Core" }
    Depends { name: "Utils" }

    files: "tst_manual_widgets_components.cpp"

    Common {}
}
