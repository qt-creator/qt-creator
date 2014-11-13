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
    autotestplugin.cpp \
    testrunner.cpp \
    testconfiguration.cpp \
    testresult.cpp \
    testresultspane.cpp \
    testresultmodel.cpp \
    testresultdelegate.cpp \
    testtreeitemdelegate.cpp

HEADERS += \
    testtreeview.h \
    testtreemodel.h \
    testtreeitem.h \
    testvisitor.h \
    testinfo.h \
    testcodeparser.h \
    autotestplugin.h \
    autotest_global.h \
    autotestconstants.h \
    testrunner.h \
    testconfiguration.h \
    testresult.h \
    testresultspane.h \
    testresultmodel.h \
    testresultdelegate.h \
    testtreeitemdelegate.h

RESOURCES += \
    autotest.qrc

