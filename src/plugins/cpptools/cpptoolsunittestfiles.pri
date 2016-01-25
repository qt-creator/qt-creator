contains(CONFIG, dll) {
    DEFINES += CPPTOOLS_LIBRARY
} else {
    DEFINES += CPPTOOLS_STATIC_LIBRARY
}

HEADERS += \
    $$PWD/cppprojectfile.h \
    $$PWD/senddocumenttracker.h \

SOURCES += \
    $$PWD/cppprojectfile.cpp \
    $$PWD/senddocumenttracker.cpp
