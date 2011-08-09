INCLUDEPATH += $$PWD/include

include($$PWD/protocol/protocol.pri)

HEADERS += \
    $$PWD/include/jsdebuggeragent.h \
    $$PWD/include/qmljsdebugger_global.h

SOURCES += \
    $$PWD/jsdebuggeragent.cpp

HEADERS += \
    $$PWD/include/qdeclarativeviewinspector.h \
    $$PWD/include/qdeclarativeviewobserver.h \
    $$PWD/include/qdeclarativeinspectorservice.h \
    $$PWD/include/qmlinspectorconstants.h \
    $$PWD/editor/abstractliveedittool.h \
    $$PWD/editor/liveselectiontool.h \
    $$PWD/editor/livelayeritem.h \
    $$PWD/editor/livesingleselectionmanipulator.h \
    $$PWD/editor/liverubberbandselectionmanipulator.h \
    $$PWD/editor/liveselectionrectangle.h \
    $$PWD/editor/liveselectionindicator.h \
    $$PWD/editor/boundingrecthighlighter.h \
    $$PWD/editor/subcomponentmasklayeritem.h \
    $$PWD/editor/zoomtool.h \
    $$PWD/editor/colorpickertool.h \
    $$PWD/qdeclarativeviewinspector_p.h

SOURCES += \
    $$PWD/qdeclarativeviewinspector.cpp \
    $$PWD/qdeclarativeinspectorservice.cpp \
    $$PWD/editor/abstractliveedittool.cpp \
    $$PWD/editor/liveselectiontool.cpp \
    $$PWD/editor/livelayeritem.cpp \
    $$PWD/editor/livesingleselectionmanipulator.cpp \
    $$PWD/editor/liverubberbandselectionmanipulator.cpp \
    $$PWD/editor/liveselectionrectangle.cpp \
    $$PWD/editor/liveselectionindicator.cpp \
    $$PWD/editor/boundingrecthighlighter.cpp \
    $$PWD/editor/subcomponentmasklayeritem.cpp \
    $$PWD/editor/zoomtool.cpp \
    $$PWD/editor/colorpickertool.cpp

DEFINES += QMLJSDEBUGGER
