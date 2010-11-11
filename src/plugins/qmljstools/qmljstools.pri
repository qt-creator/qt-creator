include(qmljstools_dependencies.pri)

DEPENDPATH += $$PWD
INCLUDEPATH += $$PWD/..

LIBS *= -l$$qtLibraryName(QmlJSTools)
