shared {
    DEFINES += CPPTOOLS_LIBRARY
} else {
    DEFINES += CPPTOOLS_STATIC_LIBRARY
}

HEADERS += \
    $$PWD/cppprojectfile.h \
    $$PWD/senddocumenttracker.h \
    $$PWD/projectpart.h

SOURCES += \
    $$PWD/cppprojectfile.cpp \
    $$PWD/senddocumenttracker.cpp \
    $$PWD/projectpart.cpp
