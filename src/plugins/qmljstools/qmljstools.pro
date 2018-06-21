include(../../qtcreatorplugin.pri)

DEFINES += QMLJSTOOLS_LIBRARY

!shared {
    DEFINES += QMLJSTOOLS_STATIC
}

HEADERS += \
    $$PWD/qmljsbundleprovider.h \
    $$PWD/qmljstoolsplugin.h \
    $$PWD/qmljstoolsconstants.h \
    $$PWD/qmljstoolssettings.h \
    $$PWD/qmljscodestylepreferencesfactory.h \
    $$PWD/qmljsmodelmanager.h \
    $$PWD/qmljsqtstylecodeformatter.h \
    $$PWD/qmljsrefactoringchanges.h \
    $$PWD/qmljsfunctionfilter.h \
    $$PWD/qmljslocatordata.h \
    $$PWD/qmljsindenter.h \
    $$PWD/qmljscodestylesettingspage.h \
    $$PWD/qmljssemanticinfo.h \
    $$PWD/qmljstools_global.h

SOURCES += \
    $$PWD/qmljsbundleprovider.cpp \
    $$PWD/qmljstoolsplugin.cpp \
    $$PWD/qmljstoolssettings.cpp \
    $$PWD/qmljscodestylepreferencesfactory.cpp \
    $$PWD/qmljsmodelmanager.cpp \
    $$PWD/qmljsqtstylecodeformatter.cpp \
    $$PWD/qmljsrefactoringchanges.cpp \
    $$PWD/qmljsfunctionfilter.cpp \
    $$PWD/qmljslocatordata.cpp \
    $$PWD/qmljsindenter.cpp \
    $$PWD/qmljscodestylesettingspage.cpp \
    $$PWD/qmljssemanticinfo.cpp

RESOURCES += \
    qmljstools.qrc

FORMS += \
    $$PWD/qmljscodestylesettingspage.ui

equals(TEST, 1) {
    SOURCES += \
        $$PWD/qmljstools_test.cpp
}
