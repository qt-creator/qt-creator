Project {
    name: "CPlusPlus Tools"
    condition: qbs.getEnv("BUILD_CPLUSPLUS_TOOLS")
    references: [
        "3rdparty/cplusplus-keywordgen/cplusplus-keywordgen.qbs",
        "cplusplus-ast2png/cplusplus-ast2png.qbs",
        "cplusplus-frontend/cplusplus-frontend.qbs",
        "cplusplus-mkvisitor/cplusplus-mkvisitor.qbs",
        "cplusplus-update-frontend/cplusplus-update-frontend.qbs",
    ]
}
