contains(CONFIG, dll) {
    DEFINES += LANGUAGEUTILS_BUILD_DIR
} else {
    DEFINES += LANGUAGEUTILS_BUILD_STATIC_LIB
}

DEPENDPATH += $$PWD
INCLUDEPATH += $$PWD/..

HEADERS += \
    $$PWD/languageutils_global.h
