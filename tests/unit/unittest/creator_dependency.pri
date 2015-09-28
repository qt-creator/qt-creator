include(../../../src/libs/sqlite/sqlite-lib.pri)
include(../../../src/libs/clangbackendipc/clangbackendipc-lib.pri)
include(../../../src/tools/clangbackend/ipcsource/clangbackendclangipc-source.pri)
include(../../../src/plugins/clangcodemodel/clangcodemodelunittestfiles.pri)
include(../../../src/plugins/cpptools/cpptoolsunittestfiles.pri)
include(cplusplus.pri)

INCLUDEPATH += \
    $$PWD/../../../src/libs \
    $$PWD/../../../src/libs/3rdparty \
    $$PWD/../../../src/plugins

DEFINES += QTC_REL_TOOLS_PATH=$$shell_quote(\"$$relative_path($$IDE_LIBEXEC_PATH, $$IDE_BIN_PATH)\")
