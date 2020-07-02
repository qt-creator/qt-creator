QT *= qml quick core

VPATH += $$PWD

INCLUDEPATH += $$PWD

SOURCES += \
    transitioneditorview.cpp \
    transitioneditorwidget.cpp \
    transitioneditortoolbar.cpp \
    transitioneditorgraphicsscene.cpp \
    transitioneditorgraphicslayout.cpp \
    transitioneditorsectionitem.cpp \
    transitioneditorpropertyitem.cpp \
    transitioneditorsettingsdialog.cpp \
    transitionform.cpp

HEADERS += \
    transitioneditorconstants.h \
    transitioneditorview.h \
    transitioneditorwidget.h \
    transitioneditortoolbar.h \
    transitioneditorgraphicsscene.h \
    transitioneditorgraphicslayout.h \
    transitioneditorsectionitem.h \
    transitioneditorpropertyitem.h \
    transitioneditorsettingsdialog.h \
    transitionform.h

RESOURCES += \
    transitioneditor.qrc

FORMS += \
    transitioneditorsettingsdialog.ui \
    transitionform.ui
