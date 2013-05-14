QTC_LIB_DEPENDS += cplusplus

include(../qttest.pri)

DEBUGGERDIR = $$IDE_SOURCE_TREE/src/plugins/debugger
DUMPERDIR   = $$IDE_SOURCE_TREE/share/qtcreator/dumper

# To access the std::type rewriter
DEFINES += CPLUSPLUS_BUILD_STATIC_LIB
include($$IDE_SOURCE_TREE/src/rpath.pri)

LIBS += -L$$IDE_PLUGIN_PATH/QtProject
DEFINES += Q_PLUGIN_PATH=\"\\\"$$IDE_PLUGIN_PATH/QtProject\\\"\"

win32 {
    CDBEXT_PATH = $$IDE_BUILD_TREE\\$$IDE_LIBRARY_BASENAME
    # replace '\' with '\\'
    DEFINES += CDBEXT_PATH=\"\\\"$$replace(CDBEXT_PATH, \\\\, \\\\)\\\"\"
} else {
    # empty string
    DEFINES += CDBEXT_PATH=\"\\\"\\\"\"
}

SOURCES += \
    $$DEBUGGERDIR/debuggerprotocol.cpp \
    $$DEBUGGERDIR/watchdata.cpp \
    $$DEBUGGERDIR/watchutils.cpp \
    tst_dumpers.cpp

HEADERS += \
    $$DEBUGGERDIR/debuggerprotocol.h \
    $$DEBUGGERDIR/watchdata.h \
    $$DEBUGGERDIR/watchutils.h \
    temporarydir.h

!isEmpty(vcproj) {
    DEFINES += DUMPERDIR=\"$$DUMPERDIR\"
} else {
    DEFINES += DUMPERDIR=\\\"$$DUMPERDIR\\\"
}

INCLUDEPATH += $$DEBUGGERDIR
DEFINES += QT_NO_CAST_FROM_ASCII
