import "../cplusplus-shared/CPlusPlusToolUsingCustomUtils.qbs" as CPlusPlusToolUsingCustomUtils

CPlusPlusToolUsingCustomUtils {
    name: "cplusplus-ast2png"
    files: base.concat(["cplusplus-ast2png.cpp"])
}
