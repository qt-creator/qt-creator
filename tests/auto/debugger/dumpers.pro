
include(../qttest.pri)

DEBUGGERDIR = $$IDE_SOURCE_TREE/src/plugins/debugger
DUMPERDIR   = $$IDE_SOURCE_TREE/share/qtcreator/dumper

# To access the std::type rewriter
DEFINES += CPLUSPLUS_BUILD_STATIC_LIB
INCLUDEPATH += $$IDE_SOURCE_TREE/src/libs/cplusplus
INCLUDEPATH += $$IDE_SOURCE_TREE/src/libs/3rdparty/cplusplus
include($$IDE_SOURCE_TREE/src/plugins/cpptools/cpptools.pri)
include($$IDE_SOURCE_TREE/src/rpath.pri)

LIBS += -L$$IDE_PLUGIN_PATH/QtProject
DEFINES += Q_PLUGIN_PATH=\"\\\"$$IDE_PLUGIN_PATH/QtProject\\\"\"

SOURCES += \
    $$DEBUGGERDIR/debuggerprotocol.cpp \
    $$DEBUGGERDIR/watchdata.cpp \
    $$DEBUGGERDIR/watchutils.cpp \
    tst_dumpers.cpp

HEADERS += \
    $$DEBUGGERDIR/debuggerprotocol.h \
    $$DEBUGGERDIR/watchdata.h \
    $$DEBUGGERDIR/watchutils.h \

!isEmpty(vcproj) {
    DEFINES += DUMPERDIR=\"$$DUMPERDIR\"
} else {
    DEFINES += DUMPERDIR=\\\"$$DUMPERDIR\\\"
}

INCLUDEPATH += $$DEBUGGERDIR
DEFINES += QT_NO_CAST_FROM_ASCII
