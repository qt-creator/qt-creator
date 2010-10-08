INCLUDEPATH += $$PWD

HEADERS += \
           $$PWD/abstractformeditortool.h \
           $$PWD/selectiontool.h \
           $$PWD/layeritem.h \
           $$PWD/singleselectionmanipulator.h \
           $$PWD/rubberbandselectionmanipulator.h \
           $$PWD/selectionrectangle.h \
           $$PWD/selectionindicator.h \
           $$PWD/boundingrecthighlighter.h \
           $$PWD/subcomponenteditortool.h \
           $$PWD/subcomponentmasklayeritem.h \
           $$PWD/zoomtool.h \
           $$PWD/colorpickertool.h \
           $$PWD/qmltoolbar.h \
           $$PWD/toolbarcolorbox.h

SOURCES += \
           $$PWD/abstractformeditortool.cpp \
           $$PWD/selectiontool.cpp \
           $$PWD/layeritem.cpp \
           $$PWD/singleselectionmanipulator.cpp \
           $$PWD/rubberbandselectionmanipulator.cpp \
           $$PWD/selectionrectangle.cpp \
           $$PWD/selectionindicator.cpp \
           $$PWD/boundingrecthighlighter.cpp \
           $$PWD/subcomponenteditortool.cpp \
           $$PWD/subcomponentmasklayeritem.cpp \
           $$PWD/zoomtool.cpp \
           $$PWD/colorpickertool.cpp \
           $$PWD/qmltoolbar.cpp \
           $$PWD/toolbarcolorbox.cpp

RESOURCES += $$PWD/editor.qrc

DEFINES += QWEAKPOINTER_ENABLE_ARROW
