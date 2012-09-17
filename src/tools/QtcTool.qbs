import qbs.base 1.0

Application {
    Depends { name: "cpp" }
    cpp.defines: project.additionalCppDefines
    cpp.linkerFlags: {
        if (qbs.buildVariant == "release" && (qbs.toolchain == "gcc" || qbs.toolchain == "mingw"))
            return ["-Wl,-s"]
    }

    destination: "bin"
}
