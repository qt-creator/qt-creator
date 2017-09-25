# Set IDE_LIBEXEC_PATH and IDE_BIN_PATH to silence a warning about empty
# QTC_REL_TOOLS_PATH, which is not used by the tests.
IDE_LIBEXEC_PATH=$$PWD
IDE_BIN_PATH=$$PWD
include($$PWD/../../../src/libs/utils/utils-lib.pri)

include($$PWD/../../../src/libs/sqlite/sqlite-lib.pri)
include($$PWD/../../../src/libs/clangsupport/clangsupport-lib.pri)
include($$PWD/../../../src/plugins/coreplugin/corepluginunittestfiles.pri)
include($$PWD/../../../src/plugins/projectexplorer/projectexplorerunittestfiles.pri)
include($$PWD/../../../src/tools/clangrefactoringbackend/source/clangrefactoringbackend-source.pri)
include($$PWD/../../../src/tools/clangpchmanagerbackend/source/clangpchmanagerbackend-source.pri)
include($$PWD/../../../src/plugins/clangrefactoring/clangrefactoring-source.pri)
include($$PWD/../../../src/plugins/clangpchmanager/clangpchmanager-source.pri)
include($$PWD/../../../src/plugins/cpptools/cpptoolsunittestfiles.pri)
include(cplusplus.pri)
!isEmpty(LLVM_INSTALL_DIR) {
include($$PWD/../../../src/shared/clang/clang_defines.pri)
include($$PWD/../../../src/tools/clangbackend/source/clangbackendclangipc-source.pri)
include($$PWD/../../../src/plugins/clangcodemodel/clangcodemodelunittestfiles.pri)
} else {
DEFINES += CLANG_VERSION=\\\"3.9.1\\\"
DEFINES += "\"CLANG_RESOURCE_DIR=\\\"/usr/include\\\"\""
}

INCLUDEPATH += \
    $$PWD/../../../src/libs \
    $$PWD/../../../src/libs/3rdparty \
    $$PWD/../../../src/plugins
