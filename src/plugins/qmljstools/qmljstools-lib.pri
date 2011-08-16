!dll {
    DEFINES += QMLJSTOOLS_STATIC
}

INCLUDEPATH += $$PWD/..

HEADERS += \
    $$PWD/qmljstools_global.h \
    $$PWD/qmljstoolsplugin.h \
    $$PWD/qmljstoolsconstants.h \
    $$PWD/qmljstoolssettings.h \
    $$PWD/qmljscodestylepreferencesfactory.h \
    $$PWD/qmljsmodelmanager.h \
    $$PWD/qmljsqtstylecodeformatter.h \
    $$PWD/qmljsrefactoringchanges.h \
    $$PWD/qmljsplugindumper.h \
    $$PWD/qmljsfunctionfilter.h \
    $$PWD/qmljslocatordata.h \
    $$PWD/qmljsindenter.h \
    $$PWD/qmljscodestylesettingspage.h \
    $$PWD/qmljsfindexportedcpptypes.h

SOURCES += \
    $$PWD/qmljstoolsplugin.cpp \
    $$PWD/qmljstoolssettings.cpp \
    $$PWD/qmljscodestylepreferencesfactory.cpp \
    $$PWD/qmljsmodelmanager.cpp \
    $$PWD/qmljsqtstylecodeformatter.cpp \
    $$PWD/qmljsrefactoringchanges.cpp \
    $$PWD/qmljsplugindumper.cpp \
    $$PWD/qmljsfunctionfilter.cpp \
    $$PWD/qmljslocatordata.cpp \
    $$PWD/qmljsindenter.cpp \
    $$PWD/qmljscodestylesettingspage.cpp \
    $$PWD/qmljsfindexportedcpptypes.cpp

FORMS += \
    $$PWD/qmljscodestylesettingspage.ui
