shared {
    DEFINES += CPPEDITOR_LIBRARY
} else {
    DEFINES += CPPEDITOR_STATIC_LIBRARY
}

HEADERS += \
    $$PWD/cppprojectfile.h

SOURCES += \
    $$PWD/cppprojectfile.cpp
