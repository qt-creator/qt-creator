HEADERS += $$PWD/cdbengine2.h \
    cdb2/bytearrayinputstream.h \
    cdb2/cdbparsehelpers.h \
    cdb2/cdboptions2.h \
    cdb2/cdboptionspage2.h

SOURCES += $$PWD/cdbengine2.cpp \
    cdb2/bytearrayinputstream.cpp \
    cdb2/cdbparsehelpers.cpp \
    cdb2/cdboptions2.cpp \
    cdb2/cdboptionspage2.cpp

FORMS += cdb2/cdboptionspagewidget2.ui

INCLUDEPATH*=$$PWD
DEPENDPATH*=$$PWD
