import "../common/common.qbs" as Common

CppApplication {
    name: "Manual Test Tracing"

    Depends { name: "Tracing"; required: false }
    Depends { name: "Utils" }
    Depends { name: "Core" }
    Depends { name: "Qt.canvaspainter"; required: false }

    condition: Tracing.present && Qt.canvaspainter.present
               && !qbs.toolchain.contains("emscripten")

    files: "tst_manual_widgets_tracing.cpp"

    Common {}
}
