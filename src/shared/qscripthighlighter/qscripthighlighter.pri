INCLUDEPATH *= $$PWD $$PWD/..

DEFINES += QSCRIPTHIGHLIGHTER_BUILD_LIB

SOURCES += $$PWD/qscriptincrementalscanner.cpp
HEADERS += $$PWD/qscriptincrementalscanner.h

contains(QT, gui) {
    SOURCES += $$PWD/qscripthighlighter.cpp $$PWD/qscriptindenter.cpp
    HEADERS += $$PWD/qscripthighlighter.h $$PWD/qscriptindenter.h
}
