import "../cplusplus-shared/CPlusPlusTool.qbs" as CPlusPlusTool

CPlusPlusTool {
    name: "cplusplus-update-frontend"

    cpp.defines: base.concat([
        'PATH_CPP_FRONTEND="' + path + '/../../libs/3rdparty/cplusplus"',
        'PATH_DUMPERS_FILE="' + path + '/../cplusplus-ast2png/dumpers.inc"',
    ])

    files: "cplusplus-update-frontend.cpp"
}
