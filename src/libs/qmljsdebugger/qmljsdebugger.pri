INCLUDEPATH += $$PWD/include
DEPENDPATH += $$PWD $$PWD/include $$PWD/editor
QT += declarative script

LIBS *= -l$$qtLibraryName(QmlJSDebugger)
