shared {
    DEFINES += DEBUGGER_LIBRARY
} else {
    DEFINES += DEBUGGER_STATIC_LIBRARY
}

HEADERS += \
    $$PWD/analyzer/diagnosticlocation.h \

SOURCES += \
    $$PWD/analyzer/diagnosticlocation.cpp \
