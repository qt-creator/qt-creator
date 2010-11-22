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
        editor/abstractformeditortool.h \
        editor/selectiontool.h \
        editor/layeritem.h \
        editor/singleselectionmanipulator.h \
        editor/rubberbandselectionmanipulator.h \
        editor/selectionrectangle.h \
        editor/selectionindicator.h \
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
        editor/abstractformeditortool.cpp \
        editor/selectiontool.cpp \
        editor/layeritem.cpp \
        editor/singleselectionmanipulator.cpp \
        editor/rubberbandselectionmanipulator.cpp \
        editor/selectionrectangle.cpp \
        editor/selectionindicator.cpp \
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
