import qbs

Product {
    name: "ProParser"
    condition: true

    Export {
        Depends { name: "cpp" }
        cpp.defines: base.concat([
            "QMAKE_AS_LIBRARY",
            "PROPARSER_THREAD_SAFE",
            "PROEVALUATOR_THREAD_SAFE",
            "PROEVALUATOR_CUMULATIVE",
            "PROEVALUATOR_SETENV",
        ])
        cpp.includePaths: base.concat([product.sourceDirectory + "/.."])
    }
}
