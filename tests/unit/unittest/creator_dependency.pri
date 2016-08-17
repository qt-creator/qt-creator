include($$PWD/../../../src/libs/utils/utils-lib.pri)
include($$PWD/../../../src/libs/sqlite/sqlite-lib.pri)
include($$PWD/../../../src/libs/clangbackendipc/clangbackendipc-lib.pri)
include($$PWD/../../../src/plugins/coreplugin/corepluginunittestfiles.pri)

!isEmpty(LLVM_INSTALL_DIR) {
include($$PWD/../../../src/tools/clangbackend/ipcsource/clangbackendclangipc-source.pri)
include($$PWD/../../../src/tools/clangrefactoringbackend/source/clangrefactoringbackend-source.pri)
include($$PWD/../../../src/plugins/clangcodemodel/clangcodemodelunittestfiles.pri)
include($$PWD/../../../src/plugins/cpptools/cpptoolsunittestfiles.pri)
include($$PWD/../../../src/plugins/clangrefactoring/clangrefactoring-source.pri)
include(cplusplus.pri)

DEFINES += CLANG_VERSION=\\\"$${LLVM_VERSION}\\\"
DEFINES += "\"CLANG_RESOURCE_DIR=\\\"$${LLVM_LIBDIR}/clang/$${LLVM_VERSION}/include\\\"\""
}

DEFINES += QTC_REL_TOOLS_PATH=$$shell_quote(\"$$relative_path($$IDE_LIBEXEC_PATH, $$IDE_BIN_PATH)\")

INCLUDEPATH += \
    $$PWD/../../../src/libs \
    $$PWD/../../../src/libs/3rdparty \
    $$PWD/../../../src/plugins
