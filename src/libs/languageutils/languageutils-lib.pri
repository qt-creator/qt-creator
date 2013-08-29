contains(CONFIG, dll) {
    DEFINES += LANGUAGEUTILS_BUILD_DIR
} else {
    DEFINES += LANGUAGEUTILS_BUILD_STATIC_LIB
}

HEADERS += \
    $$PWD/languageutils_global.h \
    $$PWD/fakemetaobject.h \
    $$PWD/componentversion.h

SOURCES += \
    $$PWD/fakemetaobject.cpp \
    $$PWD/componentversion.cpp
