import "../common/common.qbs" as Common

CppApplication {
    name: "Manual Test Tracing"

    Depends { name: "Qt.quick" }
    Depends { name: "Tracing" }
    Depends { name: "Utils" }
    Depends { name: "Core" }

    files: "tst_manual_widgets_tracing.cpp"

    Common {}
}
