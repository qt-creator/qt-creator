include(../../../src/libs/sqlite/sqlite-lib.pri)
include(../../../src/libs/clangbackendipc/clangbackendipc-lib.pri)
include(../../../src/tools/clangbackend/ipcsource/clangbackendclangipc-source.pri)
include(../../../src/tools/clangrefactoringbackend/source/clangrefactoringbackend-source.pri)
include(../../../src/plugins/coreplugin/corepluginunittestfiles.pri)
include(../../../src/plugins/clangcodemodel/clangcodemodelunittestfiles.pri)
include(../../../src/plugins/cpptools/cpptoolsunittestfiles.pri)
include(../../../src/plugins/clangrefactoring/clangrefactoring-source.pri)
include(cplusplus.pri)

INCLUDEPATH += \
    $$PWD/../../../src/libs \
    $$PWD/../../../src/libs/3rdparty \
    $$PWD/../../../src/plugins

DEFINES += QTC_REL_TOOLS_PATH=$$shell_quote(\"$$relative_path($$IDE_LIBEXEC_PATH, $$IDE_BIN_PATH)\")
DEFINES += CLANG_VERSION=\\\"$${LLVM_VERSION}\\\"
DEFINES += "\"CLANG_RESOURCE_DIR=\\\"$${LLVM_LIBDIR}/clang/$${LLVM_VERSION}/include\\\"\""
