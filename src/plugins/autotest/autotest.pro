TARGET = AutoTest
TEMPLATE = lib

include(../../qtcreatorplugin.pri)

DEFINES += AUTOTEST_LIBRARY

SOURCES += \
    autotestplugin.cpp \
    itestparser.cpp \
    projectsettingswidget.cpp \
    testcodeparser.cpp \
    testconfiguration.cpp \
    testeditormark.cpp \
    testframeworkmanager.cpp \
    testnavigationwidget.cpp \
    testoutputreader.cpp \
    testprojectsettings.cpp \
    testresult.cpp \
    testresultdelegate.cpp \
    testresultmodel.cpp \
    testresultspane.cpp \
    testrunner.cpp \
    testsettings.cpp \
    testsettingspage.cpp \
    testtreeitem.cpp \
    testtreeitemdelegate.cpp \
    testtreemodel.cpp \
    testtreeview.cpp \
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
    boost/boostcodeparser.cpp \
    boost/boosttestframework.cpp \
    boost/boosttesttreeitem.cpp \
    boost/boosttestparser.cpp \
    boost/boosttestconfiguration.cpp \
    boost/boosttestoutputreader.cpp \
    boost/boosttestresult.cpp \
    boost/boosttestsettings.cpp \
    boost/boosttestsettingspage.cpp

HEADERS += \
    autotest_global.h \
    autotestconstants.h \
    autotesticons.h \
    autotestplugin.h \
    iframeworksettings.h \
    itestframework.h \
    itestparser.h \
    itestsettingspage.h \
    projectsettingswidget.h \
    testcodeparser.h \
    testconfiguration.h \
    testeditormark.h \
    testframeworkmanager.h \
    testnavigationwidget.h \
    testoutputreader.h \
    testprojectsettings.h \
    testresult.h \
    testresultdelegate.h \
    testresultmodel.h \
    testresultspane.h \
    testrunconfiguration.h \
    testrunner.h \
    testsettings.h \
    testsettingspage.h \
    testtreeitem.h \
    testtreeitemdelegate.h \
    testtreemodel.h \
    testtreeview.h \
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
    boost/boostcodeparser.h \
    boost/boosttestframework.h \
    boost/boosttestconstants.h \
    boost/boosttesttreeitem.h \
    boost/boosttestparser.h \
    boost/boosttestconfiguration.h \
    boost/boosttestoutputreader.h \
    boost/boosttestresult.h \
    boost/boosttestsettingspage.h \
    boost/boosttestsettings.h

RESOURCES += \
    autotest.qrc

FORMS += \
    testsettingspage.ui \
    boost/boosttestsettingspage.ui \
    qtest/qttestsettingspage.ui \
    gtest/gtestsettingspage.ui

equals(TEST, 1) {
    HEADERS += autotestunittests.h
    SOURCES += autotestunittests.cpp
    RESOURCES += autotestunittests.qrc
}
