shared {
    DEFINES += PROJECTEXPLORER_LIBRARY
} else {
    DEFINES += PROJECTEXPLORER_STATIC_LIBRARY
}

HEADERS += \
    $$PWD/projectmacro.h

SOURCES += \
    $$PWD/projectmacro.cpp

