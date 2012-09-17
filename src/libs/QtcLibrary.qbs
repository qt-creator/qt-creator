import qbs.base 1.0

DynamicLibrary {
    Depends { name: "cpp" }
    cpp.defines: project.additionalCppDefines
    cpp.linkerFlags: {
        if (qbs.buildVariant == "release" && (qbs.toolchain == "gcc" || qbs.toolchain == "mingw"))
            return ["-Wl,-s"]
    }

    destination: {
        if (qbs.targetOS == "windows")
            return "bin"
        else
            return "lib/qtcreator"
    }
}
