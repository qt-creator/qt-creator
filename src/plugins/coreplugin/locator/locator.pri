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
    $$PWD/executefilter.h \
    $$PWD/locatorsearchutils.h \
    $$PWD/locatorsettingspage.h \
    $$PWD/externaltoolsfilter.h

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
    $$PWD/ilocatorfilter.cpp \
    $$PWD/executefilter.cpp \
    $$PWD/locatorsearchutils.cpp \
    $$PWD/locatorsettingspage.cpp \
    $$PWD/externaltoolsfilter.cpp

FORMS += \
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

osx {
    HEADERS += $$PWD/spotlightlocatorfilter.h
    OBJECTIVE_SOURCES += $$PWD/spotlightlocatorfilter.mm
}
