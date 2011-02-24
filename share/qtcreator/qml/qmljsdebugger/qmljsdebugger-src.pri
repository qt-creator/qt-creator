INCLUDEPATH += $$PWD/include
DEPENDPATH += $$PWD $$PWD/include editor $$PWD/qt-private

include($$PWD/protocol/protocol.pri)

HEADERS += \
    $$PWD/include/jsdebuggeragent.h \
    $$PWD/include/qmljsdebugger_global.h

SOURCES += \
    $$PWD/jsdebuggeragent.cpp

HEADERS += \
    $$PWD/include/qdeclarativeviewobserver.h \
    $$PWD/include/qdeclarativeobserverservice.h \
    $$PWD/include/qmlobserverconstants.h \
    $$PWD/editor/abstractliveedittool.h \
    $$PWD/editor/liveselectiontool.h \
    $$PWD/editor/livelayeritem.h \
    $$PWD/editor/livesingleselectionmanipulator.h \
    $$PWD/editor/liverubberbandselectionmanipulator.h \
    $$PWD/editor/liveselectionrectangle.h \
    $$PWD/editor/liveselectionindicator.h \
    $$PWD/editor/boundingrecthighlighter.h \
    $$PWD/editor/subcomponenteditortool.h \
    $$PWD/editor/subcomponentmasklayeritem.h \
    $$PWD/editor/zoomtool.h \
    $$PWD/editor/colorpickertool.h \
    $$PWD/editor/qmltoolbar.h \
    $$PWD/editor/toolbarcolorbox.h \
    $$PWD/qdeclarativeviewobserver_p.h

SOURCES += \
    $$PWD/qdeclarativeviewobserver.cpp \
    $$PWD/qdeclarativeobserverservice.cpp \
    $$PWD/editor/abstractliveedittool.cpp \
    $$PWD/editor/liveselectiontool.cpp \
    $$PWD/editor/livelayeritem.cpp \
    $$PWD/editor/livesingleselectionmanipulator.cpp \
    $$PWD/editor/liverubberbandselectionmanipulator.cpp \
    $$PWD/editor/liveselectionrectangle.cpp \
    $$PWD/editor/liveselectionindicator.cpp \
    $$PWD/editor/boundingrecthighlighter.cpp \
    $$PWD/editor/subcomponenteditortool.cpp \
    $$PWD/editor/subcomponentmasklayeritem.cpp \
    $$PWD/editor/zoomtool.cpp \
    $$PWD/editor/colorpickertool.cpp \
    $$PWD/editor/qmltoolbar.cpp \
    $$PWD/editor/toolbarcolorbox.cpp

RESOURCES += $$PWD/editor/editor.qrc

OTHER_FILES += $$PWD/qmljsdebugger.pri
