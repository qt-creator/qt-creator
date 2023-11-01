import qbs

Product {
    name: "ProParser"
    condition: true

    Export {
        Depends { name: "cpp" }
        cpp.defines: [
            "QMAKE_AS_LIBRARY",
            "PROPARSER_THREAD_SAFE",
            "PROEVALUATOR_THREAD_SAFE",
            "PROEVALUATOR_CUMULATIVE",
            "PROEVALUATOR_SETENV",
        ]
        cpp.includePaths: exportingProduct.sourceDirectory + "/.."
    }
}
