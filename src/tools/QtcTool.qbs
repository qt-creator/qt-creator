import qbs.base 1.0
import "../../qbs/defaults.js" as Defaults

Application {
    type: "application" // no Mac app bundle
    Depends { name: "cpp" }
    cpp.defines: Defaults.defines(qbs)
    cpp.linkerFlags: {
        if (qbs.buildVariant == "release" && (qbs.toolchain.contains("gcc") || qbs.toolchain.contains("mingw")))
            return ["-Wl,-s"]
    }

    Group {
        fileTagsFilter: product.type
        qbs.install: true
        qbs.installDir: project.ide_libexec_path
    }
}
