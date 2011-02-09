# This file is part of Qt Creator
# It enables debugging of Qt Quick applications

QT += declarative script
INCLUDEPATH += $$PWD/include
DEPENDPATH += $$PWD $$PWD/include editor $$PWD/qt-private

contains(CONFIG, dll) {
    DEFINES += BUILD_QMLJSDEBUGGER_LIB
} else {
    DEFINES += BUILD_QMLJSDEBUGGER_STATIC_LIB
}

!contains(DEFINES, NO_JSDEBUGGER) {

    HEADERS += \
        include/jsdebuggeragent.h \
        include/qmljsdebugger_global.h

    SOURCES += \
        jsdebuggeragent.cpp
}

!contains(DEFINES, NO_QMLOBSERVER) {

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
}

OTHER_FILES += qmljsdebugger.pri
