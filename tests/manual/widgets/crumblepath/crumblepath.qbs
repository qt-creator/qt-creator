import "../common/common.qbs" as Common

CppApplication {
    name: "Manual Test Utils CrumblePath"

    Depends { name: "Core" }
    Depends { name: "Utils" }

    files: "tst_manual_widgets_crumblepath.cpp"

    Common {}
}
