import qbs
import qbs.Environment

Project {
    name: "CPlusPlus Tools"
    condition: Environment.getEnv("BUILD_CPLUSPLUS_TOOLS")
    references: [
        "3rdparty/cplusplus-keywordgen/cplusplus-keywordgen.qbs",
        "cplusplus-ast2png/cplusplus-ast2png.qbs",
        "cplusplus-frontend/cplusplus-frontend.qbs",
        "cplusplus-mkvisitor/cplusplus-mkvisitor.qbs",
        "cplusplus-update-frontend/cplusplus-update-frontend.qbs",
    ]
}
