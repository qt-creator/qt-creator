HEADERS += \
    $$PWD/locator.h \
    $$PWD/commandlocator.h \
    $$PWD/locatorwidget.h \
    $$PWD/locatorfiltersfilter.h \
    $$PWD/settingspage.h \
    $$PWD/ilocatorfilter.h \
    $$PWD/opendocumentsfilter.h \
    $$PWD/filesystemfilter.h \
    $$PWD/locatorconstants.h \
    $$PWD/directoryfilter.h \
    $$PWD/locatormanager.h \
    $$PWD/basefilefilter.h \
    $$PWD/executefilter.h \
    $$PWD/locatorsearchutils.h

SOURCES += \
    $$PWD/locator.cpp \
    $$PWD/commandlocator.cpp \
    $$PWD/locatorwidget.cpp \
    $$PWD/locatorfiltersfilter.cpp \
    $$PWD/opendocumentsfilter.cpp \
    $$PWD/filesystemfilter.cpp \
    $$PWD/settingspage.cpp \
    $$PWD/directoryfilter.cpp \
    $$PWD/locatormanager.cpp \
    $$PWD/basefilefilter.cpp \
    $$PWD/ilocatorfilter.cpp \
    $$PWD/executefilter.cpp \
    $$PWD/locatorsearchutils.cpp

FORMS += \
    $$PWD/settingspage.ui \
    $$PWD/filesystemfilter.ui \
    $$PWD/directoryfilter.ui

RESOURCES += \
    $$PWD/locator.qrc

equals(TEST, 1) {
    HEADERS += $$PWD/locatorfiltertest.h
    SOURCES += \
        $$PWD/locatorfiltertest.cpp \
        $$PWD/locator_test.cpp
    DEFINES += SRCDIR=\\\"$$PWD\\\"
}
