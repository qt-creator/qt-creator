import qbs.base 1.0
import "../../qbs/defaults.js" as Defaults

Application {
    Depends { name: "cpp" }
    cpp.defines: Defaults.defines(qbs)
    cpp.linkerFlags: {
        if (qbs.buildVariant == "release" && (qbs.toolchain == "gcc" || qbs.toolchain == "mingw"))
            return ["-Wl,-s"]
    }

    Group {
        fileTagsFilter: product.type
        qbs.install: true
        qbs.installDir: "bin"
    }
}
