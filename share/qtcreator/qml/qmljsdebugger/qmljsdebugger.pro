# This file is part of Qt Creator
# It enables debugging of Qt Quick applications 

TEMPLATE = lib
CONFIG += staticlib
QT += declarative script
INCLUDEPATH += $$PWD/include
DEPENDPATH += $$PWD $$PWD/include editor $$PWD/qt-private

DEFINES += BUILD_QMLJSDEBUGGER_STATIC_LIB

unix:QMAKE_CXXFLAGS_DEBUG += -O3

DESTDIR = $$PWD
TARGET=qmljsdebugger
CONFIG(debug, debug|release) {
   windows:TARGET=qmljsdebuggerd
}

# JS Debugging
HEADERS += \
    include/jsdebuggeragent.h \
    include/qmljsdebugger_global.h

SOURCES += \
    jsdebuggeragent.cpp

HEADERS += \
    include/qdeclarativeviewobserver.h \
    include/qdeclarativeobserverservice.h \
    include/qmlobserverconstants.h \
    editor/abstractliveedittool.h \
    editor/liveselectiontool.h \
    editor/livelayeritem.h \
    editor/livesingleselectionmanipulator.h \
    editor/liverubberbandselectionmanipulator.h \
    editor/liveselectionrectangle.h \
    editor/liveselectionindicator.h \
    editor/boundingrecthighlighter.h \
    editor/subcomponenteditortool.h \
    editor/subcomponentmasklayeritem.h \
    editor/zoomtool.h \
    editor/colorpickertool.h \
    editor/qmltoolbar.h \
    editor/toolbarcolorbox.h \
    qdeclarativeviewobserver_p.h

SOURCES += \
    qdeclarativeviewobserver.cpp \
    qdeclarativeobserverservice.cpp \
    editor/abstractliveedittool.cpp \
    editor/liveselectiontool.cpp \
    editor/livelayeritem.cpp \
    editor/livesingleselectionmanipulator.cpp \
    editor/liverubberbandselectionmanipulator.cpp \
    editor/liveselectionrectangle.cpp \
    editor/liveselectionindicator.cpp \
    editor/boundingrecthighlighter.cpp \
    editor/subcomponenteditortool.cpp \
    editor/subcomponentmasklayeritem.cpp \
    editor/zoomtool.cpp \
    editor/colorpickertool.cpp \
    editor/qmltoolbar.cpp \
    editor/toolbarcolorbox.cpp

RESOURCES += editor/editor.qrc

OTHER_FILES += qmljsdebugger.pri
