import "../cplusplus-shared/CPlusPlusToolUsingCustomUtils.qbs" as CPlusPlusToolUsingCustomUtils

CPlusPlusToolUsingCustomUtils {
    name: "cplusplus-ast2png"
    hasCMakeProjectFile: false
    files: base.concat(["cplusplus-ast2png.cpp"])
}
