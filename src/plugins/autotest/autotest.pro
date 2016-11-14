TARGET = AutoTest
TEMPLATE = lib

include(../../qtcreatorplugin.pri)

DEFINES += AUTOTEST_LIBRARY

SOURCES += \
    testtreeview.cpp \
    testtreemodel.cpp \
    testtreeitem.cpp \
    testcodeparser.cpp \
    autotestplugin.cpp \
    testrunner.cpp \
    testconfiguration.cpp \
    testresult.cpp \
    testresultspane.cpp \
    testresultmodel.cpp \
    testresultdelegate.cpp \
    testtreeitemdelegate.cpp \
    testsettings.cpp \
    testsettingspage.cpp \
    testnavigationwidget.cpp \
    testoutputreader.cpp \
    itestparser.cpp \
    gtest/gtestconfiguration.cpp \
    gtest/gtestparser.cpp \
    gtest/gtesttreeitem.cpp \
    gtest/gtestresult.cpp \
    gtest/gtestoutputreader.cpp \
    gtest/gtestvisitors.cpp \
    gtest/gtestframework.cpp \
    gtest/gtestsettings.cpp \
    gtest/gtestsettingspage.cpp \
    gtest/gtest_utils.cpp \
    qtest/qttesttreeitem.cpp \
    qtest/qttestvisitors.cpp \
    qtest/qttestconfiguration.cpp \
    qtest/qttestoutputreader.cpp \
    qtest/qttestresult.cpp \
    qtest/qttestparser.cpp \
    qtest/qttestframework.cpp \
    qtest/qttestsettings.cpp \
    qtest/qttestsettingspage.cpp \
    qtest/qttest_utils.cpp \
    quick/quicktestconfiguration.cpp \
    quick/quicktestparser.cpp \
    quick/quicktesttreeitem.cpp \
    quick/quicktestvisitors.cpp \
    quick/quicktestframework.cpp \
    quick/quicktest_utils.cpp \
    testframeworkmanager.cpp


HEADERS += \
    testtreeview.h \
    testtreemodel.h \
    testtreeitem.h \
    testcodeparser.h \
    autotestplugin.h \
    autotest_global.h \
    autotest_utils.h \
    autotestconstants.h \
    testrunner.h \
    testconfiguration.h \
    testresult.h \
    testresultspane.h \
    testresultmodel.h \
    testresultdelegate.h \
    testtreeitemdelegate.h \
    testsettings.h \
    testsettingspage.h \
    testnavigationwidget.h \
    testoutputreader.h \
    autotesticons.h \
    itestframework.h \
    iframeworksettings.h \
    itestparser.h \
    gtest/gtestconfiguration.h \
    gtest/gtestparser.h \
    gtest/gtesttreeitem.h \
    gtest/gtestoutputreader.h \
    gtest/gtestresult.h \
    gtest/gtest_utils.h \
    gtest/gtestvisitors.h \
    gtest/gtestframework.h \
    gtest/gtestsettings.h \
    gtest/gtestsettingspage.h \
    gtest/gtestconstants.h \
    qtest/qttesttreeitem.h \
    qtest/qttest_utils.h \
    qtest/qttestresult.h \
    qtest/qttestvisitors.h \
    qtest/qttestconfiguration.h \
    qtest/qttestoutputreader.h \
    qtest/qttestparser.h \
    qtest/qttestframework.h \
    qtest/qttestsettings.h \
    qtest/qttestsettingspage.h \
    qtest/qttestconstants.h \
    quick/quicktestconfiguration.h \
    quick/quicktestparser.h \
    quick/quicktesttreeitem.h \
    quick/quicktest_utils.h \
    quick/quicktestvisitors.h \
    quick/quicktestframework.h \
    testframeworkmanager.h \
    testrunconfiguration.h \
    itestsettingspage.h

RESOURCES += \
    autotest.qrc

FORMS += \
    testsettingspage.ui \
    qtest/qttestsettingspage.ui \
    gtest/gtestsettingspage.ui

equals(TEST, 1) {
    HEADERS += autotestunittests.h
    SOURCES += autotestunittests.cpp
    RESOURCES += autotestunittests.qrc
}
