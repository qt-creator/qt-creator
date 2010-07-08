TEMPLATE = subdirs

contains(QT_CONFIG, declarative) {

    include(../../private_headers.pri)
    exists($${QT_PRIVATE_HEADERS}/QtDeclarative/private/qdeclarativemetatype_p.h) {
        SUBDIRS += qmldump qmlviewer
    } else {
        warning()
        warning("QmlDump utility has been disabled")
        warning("The helper depends on on private headers from QtDeclarative module.")
        warning("This means the Qml editor will lack correct completion and type checking.")
        warning("To enable it, pass 'QT_PRIVATE_HEADERS=$QTDIR/include' to qmake, where $QTDIR is the source directory of qt.")
        warning()
        warning("QmlViewer has been disabled")
        warning("This application depends on private headers from QtDeclarative module.")
        warning("This means the QML debugging facilities will be limited.")
        warning("To enable it, pass 'QT_PRIVATE_HEADERS=$QTDIR/include' to qmake, where $QTDIR is the source directory of qt.")
        warning()
    }
}

