import "../cplusplus-shared/CPlusPlusTool.qbs" as CPlusPlusTool

CPlusPlusTool {
    Depends { name: "Qt"; submodules: ["core", "widgets"]; }
    Depends { name: "CPlusPlus" }
    Depends { name: "Utils" }

    cpp.includePaths: base.concat(path)
    cpp.defines: base.concat('PATH_PREPROCESSOR_CONFIG="' + path + '/pp-configuration.inc"')

    files: [
        path + '/utils.h',
        path + '/utils.cpp',
    ]
}
