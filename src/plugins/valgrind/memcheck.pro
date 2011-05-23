TEMPLATE = lib
TARGET = Memcheck

DEFINES += MEMCHECK_LIBRARY

include(../../qtcreatorplugin.pri)
include(memcheck_dependencies.pri)

# Memcheck files

HEADERS += \
    memcheckplugin.h \
    memcheck_global.h \
    memchecktool.h \
    memcheckengine.h \
    memcheckerrorview.h \
    memchecksettings.h \
    memcheckconfigwidget.h \
    suppressiondialog.h

SOURCES += \
    memcheckplugin.cpp \
    memchecktool.cpp \
    memcheckengine.cpp \
    memcheckerrorview.cpp \
    memchecksettings.cpp \
    memcheckconfigwidget.cpp \
    suppressiondialog.cpp

FORMS += \
    suppressiondialog.ui \
    memcheckconfigwidget.ui \
