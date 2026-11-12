import qbs

QtcAutotest {
    name: "QtProfiler command-line recorder autotest"
    Depends { name: "Tracing"; required: false }
    Depends { name: "qtprofiler-cli"; required: false }
    condition: base && Tracing.present
    files: ["tst_profilercli.cpp"]

    Properties {
        // Names the DLL a tool that cannot be loaded is missing.
        condition: qbs.targetOS.contains("windows")
        cpp.dynamicLibraries: ["imagehlp"]
    }
}
