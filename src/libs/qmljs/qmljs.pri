INCLUDEPATH += $$PWD/../../shared
INCLUDEPATH += $$PWD/../../shared/qmljs $$PWD/../../shared/qmljs/parser

DEPENDPATH += $$PWD/../../shared/qmljs
LIBS *= -l$$qtLibraryTarget(QmlJS)
