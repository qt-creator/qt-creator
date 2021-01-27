QT *= qml

HEADERS += \
    $$PWD/locator.h \
    $$PWD/commandlocator.h \
    $$PWD/locatorwidget.h \
    $$PWD/locatorfiltersfilter.h \
    $$PWD/ilocatorfilter.h \
    $$PWD/opendocumentsfilter.h \
    $$PWD/filesystemfilter.h \
    $$PWD/locatorconstants.h \
    $$PWD/directoryfilter.h \
    $$PWD/locatormanager.h \
    $$PWD/basefilefilter.h \
    $$PWD/spotlightlocatorfilter.h \
    $$PWD/executefilter.h \
    $$PWD/locatorsearchutils.h \
    $$PWD/locatorsettingspage.h \
    $$PWD/urllocatorfilter.h \
    $$PWD/externaltoolsfilter.h \
    $$PWD/javascriptfilter.h

SOURCES += \
    $$PWD/locator.cpp \
    $$PWD/commandlocator.cpp \
    $$PWD/locatorwidget.cpp \
    $$PWD/locatorfiltersfilter.cpp \
    $$PWD/opendocumentsfilter.cpp \
    $$PWD/filesystemfilter.cpp \
    $$PWD/directoryfilter.cpp \
    $$PWD/locatormanager.cpp \
    $$PWD/basefilefilter.cpp \
    $$PWD/spotlightlocatorfilter.cpp
    $$PWD/ilocatorfilter.cpp \
    $$PWD/executefilter.cpp \
    $$PWD/locatorsearchutils.cpp \
    $$PWD/locatorsettingspage.cpp \
    $$PWD/urllocatorfilter.cpp \
    $$PWD/externaltoolsfilter.cpp \
    $$PWD/javascriptfilter.cpp

FORMS += \
    $$PWD/urllocatorfilter.ui \
    $$PWD/filesystemfilter.ui \
    $$PWD/directoryfilter.ui \
    $$PWD/locatorsettingspage.ui

equals(TEST, 1) {
    HEADERS += $$PWD/locatorfiltertest.h
    SOURCES += \
        $$PWD/locatorfiltertest.cpp \
        $$PWD/locator_test.cpp
    DEFINES += SRCDIR=\\\"$$PWD\\\"
}
