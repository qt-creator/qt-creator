INCLUDEPATH *= $$PWD
INCLUDEPATH *= $$PWD/easingpane
greaterThan(QT_MAJOR_VERSION, 4) {
    QT += quick1
} else {
    QT += declarative
}

LIBS *= -l$$qtLibraryName(QmlEditorWidgets)

include(../qmljs/qmljs.pri)
