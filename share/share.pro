TEMPLATE = subdirs
SUBDIRS = qtcreator/static.pro \
          qtcreator/translations

contains(QT_CONFIG, declarative) {
    include(../src/private_headers.pri)
    exists($${QT_PRIVATE_HEADERS}/QtDeclarative/private/qdeclarativemetatype_p.h) {
            SUBDIRS += qtcreator/qmljsdebugger
    } else {
        warning()
        warning("QmlJSDebugger library has been disabled")
        warning("This library enables extended debugging features that work together with the QML JS Inspector.")
        warning("To enable it, pass 'QT_PRIVATE_HEADERS=$QTDIR/include' to qmake, where $QTDIR is the source directory of qt.")
    }
}
