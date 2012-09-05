import qbs.base 1.0
import "../../qbs/defaults.js" as Defaults

DynamicLibrary {
    Depends { name: "cpp" }
    Depends {
        condition: Defaults.testsEnabled(qbs)
        name: "Qt.test"
    }

    cpp.defines: Defaults.defines(qbs)
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
