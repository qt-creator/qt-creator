VPATH += $$PWD

SOURCES += stateseditorwidget.cpp \
    stateseditormodel.cpp \
    stateseditorview.cpp \
    stateseditorimageprovider.cpp
HEADERS += stateseditorwidget.h \
    stateseditormodel.h \
    stateseditorview.h \
    stateseditorimageprovider.cpp
RESOURCES += $$PWD/stateseditor.qrc
OTHER_FILES +=$$PWD/stateslist.qml \
$$PWD/HorizontalScrollBar.qml
