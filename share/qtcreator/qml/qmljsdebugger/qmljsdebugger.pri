# This file is part of Qt Creator
# It enables debugging of Qt Quick applications

INCLUDEPATH += $$PWD/include
QT += declarative script

LIBS *= -l$$qtLibraryName(QmlJSDebugger)
