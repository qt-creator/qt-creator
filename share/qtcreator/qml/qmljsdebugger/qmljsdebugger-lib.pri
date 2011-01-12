# This file is part of Qt Creator
# It enables debugging of Qt Quick applications

QT += declarative
INCLUDEPATH += $$PWD/include
LIBS += -L$$PWD -lQmlJSDebugger
