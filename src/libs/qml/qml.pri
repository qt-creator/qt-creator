INCLUDEPATH += $$PWD/../../shared
INCLUDEPATH += $$PWD/../../shared/qml $$PWD/../../shared/qml/parser

DEPENDPATH += $$PWD/../../shared
LIBS *= -l$$qtLibraryTarget(Qml)
