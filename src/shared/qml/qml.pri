include(parser/parser.pri)

DEPENDPATH += $$PWD
INCLUDEPATH *= $$PWD/..

HEADERS += \
    $$PWD/qml_global.h \
    $$PWD/qmlidcollector.h \
    $$PWD/qmldocument.h \
    $$PWD/qmlsymbol.h

SOURCES += \
    $$PWD/qmlidcollector.cpp \
    $$PWD/qmldocument.cpp \
    $$PWD/qmlsymbol.cpp


