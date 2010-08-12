INCLUDEPATH += $$PWD $$PWD/easingpane
DEPENDPATH += $$PWD $$PWD/easingpane
QT += declarative

LIBS *= -l$$qtLibraryName(QmlEditorWidgets)

include(../qmljs/qmljs.pri)
