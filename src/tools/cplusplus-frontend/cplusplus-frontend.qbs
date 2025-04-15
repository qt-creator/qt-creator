import "../cplusplus-shared/CPlusPlusToolUsingCustomUtils.qbs" as CPlusPlusToolUsingCustomUtils

CPlusPlusToolUsingCustomUtils {
    name: "cplusplus-frontend"
    hasCMakeProjectFile: false
    files: base.concat("cplusplus-frontend.cpp")
}
