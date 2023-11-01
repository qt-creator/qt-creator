add_qtc_plugin(AutoTest
  PLUGIN_CLASS AutotestPlugin
  PLUGIN_DEPENDS Core CppEditor Debugger ProjectExplorer QmlJSTools TextEditor
  PLUGIN_TEST_DEPENDS QmakeProjectManager QtSupport QbsProjectManager
  SOURCES
    autotest.qrc
    autotest_global.h
    autotesttr.h
    autotestconstants.h
    autotesticons.h
    autotestplugin.cpp autotestplugin.h
    autotestunittests.qrc
    boost/boostcodeparser.cpp boost/boostcodeparser.h
    boost/boosttestconfiguration.cpp boost/boosttestconfiguration.h
    boost/boosttestconstants.h
    boost/boosttestframework.cpp boost/boosttestframework.h
    boost/boosttestoutputreader.cpp boost/boosttestoutputreader.h
    boost/boosttestparser.cpp boost/boosttestparser.h
    boost/boosttestresult.cpp boost/boosttestresult.h
    boost/boosttesttreeitem.cpp boost/boosttesttreeitem.h
    catch/catchcodeparser.cpp catch/catchcodeparser.h
    catch/catchconfiguration.cpp catch/catchconfiguration.h
    catch/catchoutputreader.cpp catch/catchoutputreader.h
    catch/catchresult.h catch/catchresult.cpp
    catch/catchtestframework.cpp catch/catchtestframework.h
    catch/catchtestparser.cpp catch/catchtestparser.h
    catch/catchtreeitem.h catch/catchtreeitem.cpp
    ctest/ctestconfiguration.cpp ctest/ctestconfiguration.h
    ctest/ctestoutputreader.cpp ctest/ctestoutputreader.h
    ctest/ctesttool.cpp ctest/ctesttool.h
    ctest/ctesttreeitem.cpp ctest/ctesttreeitem.h
    gtest/gtest_utils.cpp gtest/gtest_utils.h
    gtest/gtestconfiguration.cpp gtest/gtestconfiguration.h
    gtest/gtestconstants.h
    gtest/gtestframework.cpp gtest/gtestframework.h
    gtest/gtestoutputreader.cpp gtest/gtestoutputreader.h
    gtest/gtestparser.cpp gtest/gtestparser.h
    gtest/gtestresult.cpp gtest/gtestresult.h
    gtest/gtesttreeitem.cpp gtest/gtesttreeitem.h
    gtest/gtestvisitors.cpp gtest/gtestvisitors.h
    itemdatacache.h
    itestframework.cpp itestframework.h
    itestparser.cpp itestparser.h
    projectsettingswidget.cpp projectsettingswidget.h
    qtest/qttest_utils.cpp qtest/qttest_utils.h
    qtest/qttestconfiguration.cpp qtest/qttestconfiguration.h
    qtest/qttestconstants.h
    qtest/qttestframework.cpp qtest/qttestframework.h
    qtest/qttestoutputreader.cpp qtest/qttestoutputreader.h
    qtest/qttestparser.cpp qtest/qttestparser.h
    qtest/qttestresult.cpp qtest/qttestresult.h
    qtest/qttesttreeitem.cpp qtest/qttesttreeitem.h
    qtest/qttestvisitors.cpp qtest/qttestvisitors.h
    quick/quicktest_utils.cpp quick/quicktest_utils.h
    quick/quicktestconfiguration.cpp quick/quicktestconfiguration.h
    quick/quicktestframework.cpp quick/quicktestframework.h
    quick/quicktestparser.cpp quick/quicktestparser.h
    quick/quicktesttreeitem.cpp quick/quicktesttreeitem.h
    quick/quicktestvisitors.cpp quick/quicktestvisitors.h
    testcodeparser.cpp testcodeparser.h
    testconfiguration.cpp testconfiguration.h
    testeditormark.cpp testeditormark.h
    testframeworkmanager.cpp testframeworkmanager.h
    testnavigationwidget.cpp testnavigationwidget.h
    testoutputreader.cpp testoutputreader.h
    testprojectsettings.cpp testprojectsettings.h
    testresult.cpp testresult.h
    testresultdelegate.cpp testresultdelegate.h
    testresultmodel.cpp testresultmodel.h
    testresultspane.cpp testresultspane.h
    testrunconfiguration.h
    testrunner.cpp testrunner.h
    testsettings.cpp testsettings.h
    testsettingspage.cpp testsettingspage.h
    testtreeitem.cpp testtreeitem.h
    testtreeitemdelegate.cpp testtreeitemdelegate.h
    testtreemodel.cpp testtreemodel.h
    testtreeview.cpp testtreeview.h
)

extend_qtc_plugin(AutoTest
  CONDITION WITH_TESTS
  SOURCES
    autotestunittests.cpp autotestunittests.h
    loadprojectscenario.cpp loadprojectscenario.h
)
