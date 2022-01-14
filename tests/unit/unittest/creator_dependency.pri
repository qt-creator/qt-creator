include($$PWD/../../../src/libs/utils/utils-lib.pri)

include($$PWD/../../../src/libs/3rdparty/yaml-cpp/yaml-cpp.pri)
include($$PWD/../../../src/libs/sqlite/sqlite-lib.pri)
include($$PWD/../../../src/libs/clangsupport/clangsupport-lib.pri)
include($$PWD/../../../src/plugins/coreplugin/corepluginunittestfiles.pri)
include($$PWD/../../../src/plugins/cppeditor/cppeditorunittestfiles.pri)
include($$PWD/../../../src/plugins/debugger/debuggerunittestfiles.pri)
include($$PWD/../../../src/plugins/qmldesigner/qmldesignerunittestfiles.pri)
!isEmpty(QTC_UNITTEST_BUILD_CPP_PARSER):include(cplusplus.pri)
!isEmpty(LLVM_VERSION) {
include($$PWD/../../../src/plugins/clangtools/clangtoolsunittestfiles.pri)
include($$PWD/../../../src/shared/clang/clang_defines.pri)
include($$PWD/../../../src/tools/clangbackend/source/clangbackendclangipc-source.pri)
include($$PWD/../../../src/plugins/clangcodemodel/clangcodemodelunittestfiles.pri)
} else {
DEFINES += CLANG_VERSION=\\\"6.0.0\\\"
DEFINES += "\"CLANG_INCLUDE_DIR=\\\"/usr/include\\\"\""
}

INCLUDEPATH += \
    $$PWD/../../../src/libs \
    $$PWD/../../../src/libs/3rdparty \
    $$PWD/../../../src/plugins
