
INCLUDEPATH *= $$PWD/../..
macx {
    LIBPATH*= $$PWD/../../../../bin/QtCreator.app/Contents/PlugIns
}
else {
    LIBPATH*= $$PWD/../../../../lib
}

include(../extensionsystem.pri)

QT *= xml
