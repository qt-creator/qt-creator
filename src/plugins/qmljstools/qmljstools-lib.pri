!dll {
    DEFINES += QMLJSTOOLS_STATIC
}

INCLUDEPATH += $$PWD/..

HEADERS += \
    $$PWD/qmljstools_global.h \
    $$PWD/qmljstoolsplugin.h \
    $$PWD/qmljstoolsconstants.h \
    $$PWD/qmljsmodelmanager.h \
    $$PWD/qmljsqtstylecodeformatter.h \
    $$PWD/qmljsrefactoringchanges.h \
    $$PWD/qmljsplugindumper.h \
    $$PWD/qmljsfunctionfilter.h \
    $$PWD/qmljslocatordata.h

SOURCES += \
    $$PWD/qmljstoolsplugin.cpp \
    $$PWD/qmljsmodelmanager.cpp \
    $$PWD/qmljsqtstylecodeformatter.cpp \
    $$PWD/qmljsrefactoringchanges.cpp \
    $$PWD/qmljsplugindumper.cpp \
    $$PWD/qmljsfunctionfilter.cpp \
    $$PWD/qmljslocatordata.cpp
