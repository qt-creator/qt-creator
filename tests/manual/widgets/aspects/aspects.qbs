import "../common/common.qbs" as Common

CppApplication {
    name: "Manual Test Utils Aspects"

    Depends { name: "Core" }
    Depends { name: "ProjectExplorer" }
    Depends { name: "Utils" }

    files: "tst_manual_widgets_aspects.cpp"

    Common {}
}
