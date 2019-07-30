shared {
    DEFINES += CLANGTOOLS_LIBRARY
} else {
    DEFINES += CLANGTOOLS_STATIC_LIBRARY
}

HEADERS += \
    $$PWD/clangtoolsdiagnostic.h \
    $$PWD/clangtoolslogfilereader.h \

SOURCES += \
    $$PWD/clangtoolsdiagnostic.cpp \
    $$PWD/clangtoolslogfilereader.cpp \
