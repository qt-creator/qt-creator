include(../../../src/libs/sqlite/sqlite-lib.pri)
include(../../../src/libs/clangbackendipc/clangbackendipc-lib.pri)
include(../../../src/tools/clangbackend/ipcsource/clangbackendclangipc-source.pri)
include(../../../src/plugins/clangcodemodel/clangcodemodelunittestfiles.pri)
include(cplusplus.pri)

INCLUDEPATH += \
    $$PWD/../../../src/libs \
    $$PWD/../../../src/libs/3rdparty \
    $$PWD/../../../src/plugins
