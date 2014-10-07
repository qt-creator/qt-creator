TARGET = AutoTest
TEMPLATE = lib

PROVIDER = Digia

include(../../qtcreatorplugin.pri)
include(autotest_dependencies.pri)

DEFINES += AUTOTEST_LIBRARY

SOURCES += \
    testtreeview.cpp \
    testtreemodel.cpp \
    testtreeitem.cpp \
    testvisitor.cpp \
    testinfo.cpp \
    testcodeparser.cpp \
    autotestplugin.cpp

HEADERS += \
    testtreeview.h \
    testtreemodel.h \
    testtreeitem.h \
    testvisitor.h \
    testinfo.h \
    testcodeparser.h \
    autotestplugin.h \
    autotest_global.h \
    autotestconstants.h

RESOURCES += \
    autotest.qrc

