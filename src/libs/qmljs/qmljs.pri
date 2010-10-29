INCLUDEPATH += $$PWD/../../shared
INCLUDEPATH += $$PWD/../../shared/qmljs $$PWD/../../shared/qmljs/parser

DEPENDPATH += $$PWD/../../shared/qmljs $$PWD/../../shared/qmljs/parser
LIBS *= -l$$qtLibraryName(QmlJS)
DEFINES += QT_CREATOR
