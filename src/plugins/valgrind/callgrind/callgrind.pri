QT *= network

HEADERS += \
    $$PWD/callgrindparser.h \
    $$PWD/callgrindparsedata.h \
    $$PWD/callgrindfunction.h \
    $$PWD/callgrindfunction_p.h \
    $$PWD/callgrindfunctioncycle.h \
    $$PWD/callgrindfunctioncall.h \
    $$PWD/callgrindcostitem.h \
    $$PWD/callgrinddatamodel.h \
    $$PWD/callgrindabstractmodel.h \
    $$PWD/callgrindcallmodel.h \
    $$PWD/callgrindcontroller.h \
    $$PWD/callgrindcycledetection.h \
    $$PWD/callgrindproxymodel.h \
    $$PWD/callgrindstackbrowser.h

SOURCES += \
    $$PWD/callgrindparser.cpp \
    $$PWD/callgrindparsedata.cpp \
    $$PWD/callgrindfunction.cpp \
    $$PWD/callgrindfunctioncycle.cpp \
    $$PWD/callgrindfunctioncall.cpp \
    $$PWD/callgrindcostitem.cpp \
    $$PWD/callgrinddatamodel.cpp \
    $$PWD/callgrindcallmodel.cpp \
    $$PWD/callgrindcontroller.cpp \
    $$PWD/callgrindcycledetection.cpp \
    $$PWD/callgrindproxymodel.cpp \
    $$PWD/callgrindstackbrowser.cpp
