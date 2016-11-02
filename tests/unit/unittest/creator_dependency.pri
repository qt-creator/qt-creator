include($$PWD/../../../src/libs/utils/utils-lib.pri)
include($$PWD/../../../src/libs/sqlite/sqlite-lib.pri)
include($$PWD/../../../src/libs/clangbackendipc/clangbackendipc-lib.pri)
include($$PWD/../../../src/plugins/coreplugin/corepluginunittestfiles.pri)

!isEmpty(LLVM_INSTALL_DIR) {
include($$PWD/../../../src/shared/clang/clang_defines.pri)

include($$PWD/../../../src/tools/clangbackend/ipcsource/clangbackendclangipc-source.pri)
include($$PWD/../../../src/tools/clangrefactoringbackend/source/clangrefactoringbackend-source.pri)
include($$PWD/../../../src/plugins/clangcodemodel/clangcodemodelunittestfiles.pri)
include($$PWD/../../../src/plugins/cpptools/cpptoolsunittestfiles.pri)
include($$PWD/../../../src/plugins/clangrefactoring/clangrefactoring-source.pri)
include(cplusplus.pri)
}

INCLUDEPATH += \
    $$PWD/../../../src/libs \
    $$PWD/../../../src/libs/3rdparty \
    $$PWD/../../../src/plugins
