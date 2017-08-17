shared|dll {
    DEFINES += CLANGPCHMANAGER_LIB
} else {
    DEFINES += CLANGPCHMANAGER_STATIC_LIB
}

INCLUDEPATH += $$PWD

HEADERS += \
    $$PWD/pchmanagerclient.h \
    $$PWD/pchmanagernotifierinterface.h \
    $$PWD/pchmanagerconnectionclient.h \
    $$PWD/clangpchmanager_global.h \
    $$PWD/projectupdater.h \
    $$PWD/pchmanagerprojectupdater.h


SOURCES += \
    $$PWD/pchmanagerclient.cpp \
    $$PWD/pchmanagernotifierinterface.cpp \
    $$PWD/pchmanagerconnectionclient.cpp \
    $$PWD/projectupdater.cpp \
    $$PWD/pchmanagerprojectupdater.cpp

