!dll {
    DEFINES += QMLJSTOOLS_STATIC
}

DEPENDPATH += $$PWD
INCLUDEPATH += $$PWD/..

HEADERS += \
    $$PWD/qmljstools_global.h \
    $$PWD/qmljstoolsplugin.h \
    $$PWD/qmljstoolsconstants.h \
    $$PWD/qmljsmodelmanager.h \
    $$PWD/qmljsqtstylecodeformatter.h \
    $$PWD/qmljsrefactoringchanges.h

SOURCES += \
    $$PWD/qmljstoolsplugin.cpp \
    $$PWD/qmljsmodelmanager.cpp \
    $$PWD/qmljsqtstylecodeformatter.cpp \
    $$PWD/qmljsrefactoringchanges.cpp
