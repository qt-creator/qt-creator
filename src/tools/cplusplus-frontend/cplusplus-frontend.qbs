import "../cplusplus-shared/CPlusPlusToolUsingCustomUtils.qbs" as CPlusPlusToolUsingCustomUtils

CPlusPlusToolUsingCustomUtils {
    name: "cplusplus-frontend"
    files: base.concat("cplusplus-frontend.cpp")
}
