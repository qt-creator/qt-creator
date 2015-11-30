QT *= network

INCLUDEPATH *= $$PWD

HEADERS += \
    $$PWD/xmlprotocol/frame.h \
    $$PWD/xmlprotocol/parser.h \
    $$PWD/xmlprotocol/error.h \
    $$PWD/xmlprotocol/status.h \
    $$PWD/xmlprotocol/suppression.h \
    $$PWD/xmlprotocol/threadedparser.h \
    $$PWD/xmlprotocol/announcethread.h \
    $$PWD/xmlprotocol/stack.h \
    $$PWD/xmlprotocol/errorlistmodel.h \
    $$PWD/xmlprotocol/stackmodel.h \
    $$PWD/xmlprotocol/modelhelpers.h \
    $$PWD/callgrind/callgrindparser.h \
    $$PWD/callgrind/callgrindparsedata.h \
    $$PWD/callgrind/callgrindfunction.h \
    $$PWD/callgrind/callgrindfunction_p.h \
    $$PWD/callgrind/callgrindfunctioncycle.h \
    $$PWD/callgrind/callgrindfunctioncall.h \
    $$PWD/callgrind/callgrindcostitem.h \
    $$PWD/callgrind/callgrinddatamodel.h \
    $$PWD/callgrind/callgrindcallmodel.h \
    $$PWD/callgrind/callgrindcontroller.h \
    $$PWD/callgrind/callgrindcycledetection.h \
    $$PWD/callgrind/callgrindproxymodel.h \
    $$PWD/callgrind/callgrindstackbrowser.h \
    $$PWD/callgrind/callgrindrunner.h \
    $$PWD/memcheck/memcheckrunner.h \
    $$PWD/valgrindrunner.h \
    $$PWD/valgrindprocess.h

SOURCES += $$PWD/xmlprotocol/error.cpp \
    $$PWD/xmlprotocol/frame.cpp \
    $$PWD/xmlprotocol/parser.cpp \
    $$PWD/xmlprotocol/status.cpp \
    $$PWD/xmlprotocol/suppression.cpp \
    $$PWD/xmlprotocol/threadedparser.cpp \
    $$PWD/xmlprotocol/announcethread.cpp \
    $$PWD/xmlprotocol/stack.cpp \
    $$PWD/xmlprotocol/errorlistmodel.cpp \
    $$PWD/xmlprotocol/stackmodel.cpp \
    $$PWD/xmlprotocol/modelhelpers.cpp \
    $$PWD/callgrind/callgrindparser.cpp \
    $$PWD/callgrind/callgrindparsedata.cpp \
    $$PWD/callgrind/callgrindfunction.cpp \
    $$PWD/callgrind/callgrindfunctioncycle.cpp \
    $$PWD/callgrind/callgrindfunctioncall.cpp \
    $$PWD/callgrind/callgrindcostitem.cpp \
    $$PWD/callgrind/callgrinddatamodel.cpp \
    $$PWD/callgrind/callgrindcallmodel.cpp \
    $$PWD/callgrind/callgrindcontroller.cpp \
    $$PWD/callgrind/callgrindcycledetection.cpp \
    $$PWD/callgrind/callgrindproxymodel.cpp \
    $$PWD/callgrind/callgrindrunner.cpp \
    $$PWD/callgrind/callgrindstackbrowser.cpp \
    $$PWD/memcheck/memcheckrunner.cpp \
    $$PWD/valgrindrunner.cpp \
    $$PWD/valgrindprocess.cpp

LIBS += -L$$IDE_PLUGIN_PATH/QtProject
