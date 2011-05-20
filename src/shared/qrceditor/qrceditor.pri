INCLUDEPATH *= $$PWD $$PWD/..
DEPENDPATH *= $$PWD $$PWD/..

QT *= xml

# Input
SOURCES += \
    $$PWD/resourcefile.cpp \
    $$PWD/resourceview.cpp \
    $$PWD/qrceditor.cpp \
    $$PWD/undocommands.cpp \

HEADERS += \
    $$PWD/resourcefile_p.h \
    $$PWD/resourceview.h \
    $$PWD/qrceditor.h \
    $$PWD/undocommands_p.h

FORMS += $$PWD/qrceditor.ui

