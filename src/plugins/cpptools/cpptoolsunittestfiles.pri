contains(CONFIG, dll) {
    DEFINES += CPPTOOLS_LIBRARY
} else {
    DEFINES += CPPTOOLS_STATIC_LIBRARY
}

HEADERS += $$PWD/senddocumenttracker.h

SOURCES += $$PWD/senddocumenttracker.cpp
