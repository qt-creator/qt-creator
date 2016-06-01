TEMPLATE = lib

SOURCES += plugin3.cpp
HEADERS += plugin3.h
OTHER_FILES += plugin3.json

QTC_LIB_DEPENDS += extensionsystem
include(../../../../auto/qttest.pri)

TARGET = $$qtLibraryTargetName(plugin3)

LIBS += -L$${OUT_PWD}/../plugin2 -l$$qtLibraryName(plugin2)

osx {
    QMAKE_LFLAGS_SONAME = -Wl,-install_name,$${OUT_PWD}/
} else:unix {
    QMAKE_RPATHDIR += $${OUT_PWD}/../plugin2
} else:win32 {
    DESTDIR = $$OUT_PWD
}
