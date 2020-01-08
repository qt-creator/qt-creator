QT = core network

msvc: QTC_LIB_DEPENDS += utils

include(../qttest.pri)

msvc {

    LIBS *= -L$$IDE_PLUGIN_PATH
    DEFINES += Q_PLUGIN_PATH=\"\\\"$$IDE_PLUGIN_PATH\\\"\"

    CDBEXT_PATH = $$IDE_BUILD_TREE\\$$IDE_LIBRARY_BASENAME
    # replace '\' with '\\'
    DEFINES += CDBEXT_PATH=\"\\\"$$replace(CDBEXT_PATH, \\\\, \\\\)\\\"\"

} else {

    SOURCES += \
        $$IDE_SOURCE_TREE/src/libs/utils/environment.cpp \
        $$IDE_SOURCE_TREE/src/libs/utils/fileutils.cpp \
        $$IDE_SOURCE_TREE/src/libs/utils/hostosinfo.cpp \
        $$IDE_SOURCE_TREE/src/libs/utils/namevaluedictionary.cpp \
        $$IDE_SOURCE_TREE/src/libs/utils/namevalueitem.cpp \
        $$IDE_SOURCE_TREE/src/libs/utils/treemodel.cpp \
        $$IDE_SOURCE_TREE/src/libs/utils/qtcassert.cpp \
        $$IDE_SOURCE_TREE/src/libs/utils/qtcprocess.cpp \
        $$IDE_SOURCE_TREE/src/libs/utils/processhandle.cpp \
        $$IDE_SOURCE_TREE/src/libs/utils/savefile.cpp

    HEADERS += \
        $$IDE_SOURCE_TREE/src/libs/utils/environment.h \
        $$IDE_SOURCE_TREE/src/libs/utils/fileutils.h \
        $$IDE_SOURCE_TREE/src/libs/utils/hostosinfo.h \
        $$IDE_SOURCE_TREE/src/libs/utils/namevaluedictionary.h \
        $$IDE_SOURCE_TREE/src/libs/utils/namevalueitem.h \
        $$IDE_SOURCE_TREE/src/libs/utils/treemodel.h \
        $$IDE_SOURCE_TREE/src/libs/utils/qtcassert.h \
        $$IDE_SOURCE_TREE/src/libs/utils/qtcprocess.h \
        $$IDE_SOURCE_TREE/src/libs/utils/processhandle.h \
        $$IDE_SOURCE_TREE/src/libs/utils/savefile.h

    macos: {
        HEADERS += $$IDE_SOURCE_TREE/src/libs/utils/fileutils_mac.h
        OBJECTIVE_SOURCES += $$IDE_SOURCE_TREE/src/libs/utils/fileutils_mac.mm
        LIBS += -framework Foundation
    }
}

DEBUGGERDIR = $$IDE_SOURCE_TREE/src/plugins/debugger
DUMPERDIR   = $$IDE_SOURCE_TREE/share/qtcreator/debugger

SOURCES += \
    $$DEBUGGERDIR/debuggerprotocol.cpp \
    $$DEBUGGERDIR/simplifytype.cpp \
    $$DEBUGGERDIR/watchdata.cpp \
    $$DEBUGGERDIR/watchutils.cpp \
    tst_dumpers.cpp

HEADERS += \
    $$DEBUGGERDIR/debuggerprotocol.h \
    $$DEBUGGERDIR/simplifytype.h \
    $$DEBUGGERDIR/watchdata.h \
    $$DEBUGGERDIR/watchutils.h

!isEmpty(vcproj) {
    DEFINES += DUMPERDIR=\"$$DUMPERDIR\"
} else {
    DEFINES += DUMPERDIR=\\\"$$DUMPERDIR\\\"
}

INCLUDEPATH += $$DEBUGGERDIR

# clang 3.5 does not like to optimize long functions.
# likewise gcc 5.4
clang|gcc: QMAKE_CXXFLAGS_RELEASE =
