INCLUDEPATH += $$PWD/include
DEPENDPATH += $$PWD $$PWD/include $$PWD/editor
QT += declarative

LIBS *= -l$$qtLibraryTarget(QmlJSDebugger)
