TEMPLATE = lib

SOURCES += plugin2.cpp
HEADERS += plugin2.h
OTHER_FILES += plugin2.json

QTC_LIB_DEPENDS += extensionsystem
include(../../../../auto/qttest.pri)

TARGET = $$qtLibraryTargetName(plugin2)

osx {
    QMAKE_LFLAGS_SONAME = -Wl,-install_name,$${OUT_PWD}/
} else:win32 {
    DESTDIR = $$OUT_PWD
}

