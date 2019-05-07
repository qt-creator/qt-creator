shared|dll {
    DEFINES += CLANGPCHMANAGER_LIB
} else {
    DEFINES += CLANGPCHMANAGER_STATIC_LIB
}

INCLUDEPATH += $$PWD

HEADERS += \
    $$PWD/clangindexingprojectsettings.h \
    $$PWD/clangindexingsettingsmanager.h \
    $$PWD/pchmanagerclient.h \
    $$PWD/pchmanagernotifierinterface.h \
    $$PWD/pchmanagerconnectionclient.h \
    $$PWD/clangpchmanager_global.h \
    $$PWD/preprocessormacrocollector.h \
    $$PWD/projectupdater.h \
    $$PWD/pchmanagerprojectupdater.h \
    $$PWD/progressmanager.h \
    $$PWD/progressmanagerinterface.h

SOURCES += \
    $$PWD/clangindexingprojectsettings.cpp \
    $$PWD/clangindexingsettingsmanager.cpp \
    $$PWD/pchmanagerclient.cpp \
    $$PWD/pchmanagernotifierinterface.cpp \
    $$PWD/pchmanagerconnectionclient.cpp \
    $$PWD/preprocessormacrocollector.cpp \
    $$PWD/projectupdater.cpp \
    $$PWD/pchmanagerprojectupdater.cpp
