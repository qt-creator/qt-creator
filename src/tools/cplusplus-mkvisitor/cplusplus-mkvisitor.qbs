import "../cplusplus-shared/CPlusPlusToolUsingCustomUtils.qbs" as CPlusPlusToolUsingCustomUtils

CPlusPlusToolUsingCustomUtils {
    name: "cplusplus-mkvisitor"
    cpp.defines: base.concat('PATH_AST_H="' + path + '/../../libs/3rdparty/cplusplus/AST.h"')
    files: base.concat("cplusplus-mkvisitor.cpp")
}
