TEMPLATE = lib

SOURCES += plugin1.cpp
HEADERS += plugin1.h
OTHER_FILES += plugin1.json

QTC_LIB_DEPENDS += extensionsystem
include(../../../../auto/qttest.pri)

TARGET = $$qtLibraryTargetName(plugin1)

LIBS += -L$${OUT_PWD}/../plugin2 -L$${OUT_PWD}/../plugin3 -l$$qtLibraryTargetName(plugin2) -l$$qtLibraryTargetName(plugin3)

osx {
} else:unix {
    QMAKE_RPATHDIR += $${OUT_PWD}/../plugin2
    QMAKE_RPATHDIR += $${OUT_PWD}/../plugin3
}
