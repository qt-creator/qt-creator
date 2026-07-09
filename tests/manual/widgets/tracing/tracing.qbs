import "../common/common.qbs" as Common

CppApplication {
    name: "Manual Test Tracing"

    Depends { name: "Qt.quick" }
    Depends { name: "Tracing"; required: false }
    Depends { name: "Utils" }
    Depends { name: "Core" }

    condition: Tracing.present

    files: "tst_manual_widgets_tracing.cpp"

    Common {}
}
