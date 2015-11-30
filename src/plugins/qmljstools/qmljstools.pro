include(../../qtcreatorplugin.pri)

DEFINES += QMLJSTOOLS_LIBRARY

!dll {
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
    $$PWD/qmljstools_global.h \
    $$PWD/qmlconsolemanager.h \
    $$PWD/qmlconsoleitemmodel.h \
    $$PWD/qmlconsolemodel.h \
    $$PWD/qmlconsolepane.h \
    $$PWD/qmlconsoleview.h \
    $$PWD/qmlconsoleitemdelegate.h \
    $$PWD/qmlconsoleedit.h \
    $$PWD/qmljsinterpreter.h \
    $$PWD/qmlconsoleproxymodel.h

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
    $$PWD/qmljssemanticinfo.cpp \
    $$PWD/qmlconsolemanager.cpp \
    $$PWD/qmlconsoleitemmodel.cpp \
    $$PWD/qmlconsolepane.cpp \
    $$PWD/qmlconsoleview.cpp \
    $$PWD/qmlconsoleitemdelegate.cpp \
    $$PWD/qmlconsoleedit.cpp \
    $$PWD/qmljsinterpreter.cpp \
    $$PWD/qmlconsoleproxymodel.cpp

RESOURCES += \
    qmljstools.qrc

FORMS += \
    $$PWD/qmljscodestylesettingspage.ui

equals(TEST, 1) {
    SOURCES += \
        $$PWD/qmljstools_test.cpp
}
