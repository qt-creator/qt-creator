WITH_LLDB = $$(WITH_LLDB)

!macx: error (This can only be built on mac)
!exists($${WITH_LLDB}/include/lldb/lldb-enumerations.h): error(please see the README for build instructions)

QT = core network

include(../../../../../qtcreator.pri)
TEMPLATE = app
CONFIG  -= app_bundle
CONFIG += debug
TARGET = qtcreator-lldb
DEPENDPATH += . .. ../.. ../../..
INCLUDEPATH += . .. ../.. ../../..
DESTDIR  = $$IDE_LIBEXEC_PATH

MOC_DIR=.tmp
OBJECTS_DIR=.tmp

HEADERS += ../ipcengineguest.h \
            ../debuggerstreamops.h \
            ../breakpoint.h \
            ../watchdata.h \
            ../stackframe.h \
            ../disassemblerlines.h \
            lldbengineguest.h

SOURCES +=  ../ipcengineguest.cpp \
            ../debuggerstreamops.cpp \
            ../breakpoint.cpp \
            ../watchdata.cpp \
            ../stackframe.cpp \
            ../disassemblerlines.cpp \
            lldbengineguest.cpp \
            main.cpp


LIBS += -sectcreate __TEXT __info_plist $$PWD/qtcreator-lldb.plist

POSTL = rm -rf \'$${IDE_LIBEXEC_PATH}/LLDB.framework\' $$escape_expand(\\n\\t) \
        $$QMAKE_COPY_DIR $${WITH_LLDB}/build/Release/* \'$$IDE_LIBEXEC_PATH\' $$escape_expand(\\n\\t) \
        install_name_tool -change '@rpath/LLDB.framework/Versions/A/LLDB' '@executable_path/LLDB.framework/Versions/A/LLDB' $(TARGET)  $$escape_expand(\\n\\t) \
        codesign -s lldb_codesign $(TARGET)

!isEmpty(QMAKE_POST_LINK):QMAKE_POST_LINK = $$escape_expand(\\n\\t)$$QMAKE_POST_LINK
QMAKE_POST_LINK = $$POSTL $$QMAKE_POST_LINK
silent:QMAKE_POST_LINK = @echo signing $@ && $$QMAKE_POST_LINK

LIBS +=  -framework Security -framework Python

DEFINES += __STDC_LIMIT_MACROS __STDC_CONSTANT_MACROS

INCLUDEPATH += $${WITH_LLDB}/include $${WITH_LLDB}/llvm/include/
LIBS += -F$${WITH_LLDB}/build/Release -framework LLDB

# include (lldb.pri)
# DEFINES += HAVE_LLDB_PRIVATE
# HEADERS += pygdbmiemu.h
# SOURCES += pygdbmiemu.cpp


