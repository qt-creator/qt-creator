@if "%{TestFrameWork}" == "QtTest"
QT += testlib
@if "%{RequireGUI}" == "false"
QT -= gui

CONFIG += qt console warn_on depend_includepath testcase
CONFIG -= app_bundle
@else
QT += gui
CONFIG += qt warn_on depend_includepath testcase
@endif

TEMPLATE = app

SOURCES +=  %{TestCaseFileWithCppSuffix}
@endif
@if "%{TestFrameWork}" == "QtQuickTest"
CONFIG += warn_on qmltestcase

TEMPLATE = app

DISTFILES += \\
    %{TestCaseFileWithQmlSuffix}

@if "%{UseSetupCode}" === "false"
SOURCES += \\
    %{MainCppName}
@else
HEADERS += setup.h

SOURCES += \\
    %{MainCppName} \\
    setup.cpp
@endif
@endif
@if "%{TestFrameWork}" == "GTest" || "%{TestFrameWork}" == "GTest_dyn"
include(gtest_dependency.pri)

TEMPLATE = app
CONFIG += console c++14
CONFIG -= app_bundle
CONFIG += thread
CONFIG -= qt

SOURCES += \\
        %{MainCppName} \\
        %{TestCaseFileGTestWithCppSuffix}
@endif
@if "%{TestFrameWork}" == "BoostTest"
TEMPLATE = app
CONFIG -= qt
CONFIG -= app_bundle
CONFIG += console

isEmpty(BOOST_INCLUDE_DIR): BOOST_INCLUDE_DIR=$$(BOOST_INCLUDE_DIR)
@if "%{BoostIncDir}" != ""
# set by Qt Creator wizard
isEmpty(BOOST_INCLUDE_DIR): BOOST_INCLUDE_DIR="%{BoostIncDir}"
@endif
!isEmpty(BOOST_INCLUDE_DIR): INCLUDEPATH *= $${BOOST_INCLUDE_DIR}

isEmpty(BOOST_INCLUDE_DIR): {
    message("BOOST_INCLUDE_DIR is not set, assuming Boost can be found automatically in your system")
}

SOURCES += \\
    %{MainCppName}
@endif
@if "%{TestFrameWork}" == "BoostTest_dyn"
TEMPLATE = app
CONFIG -= qt
CONFIG -= app_bundle
CONFIG += console

isEmpty(BOOST_INSTALL_DIR): BOOST_INSTALL_DIR=$$(BOOST_INSTALL_DIR)
@if "%{BoostInstallDir}" != ""
# set by Qt Creator wizard
isEmpty(BOOST_INSTALL_DIR): BOOST_INSTALL_DIR="%{BoostInstallDir}"
@endif
!isEmpty(BOOST_INSTALL_DIR) {
  win32: INCLUDEPATH *= $${BOOST_INSTALL_DIR}
  else: INCLUDEPATH *= $${BOOST_INSTALL_DIR}/include
}
# Windows: adapt to name scheme, e.g. lib64-msvc-14.2
!isEmpty(BOOST_INSTALL_DIR): LIBS *= -L$${BOOST_INSTALL_DIR}/lib
# Windows: adapt to name scheme, e.g. boost_unit_test_framework-vc142-mt-gd-x64-1_80
LIBS *= -lboost_unit_test_framework
DEFINES *= BOOST_UNIT_TEST_FRAMEWORK_DYN_LINK

isEmpty(BOOST_INSTALL_DIR): {
    message("BOOST_INSTALL_DIR is not set, assuming Boost can be found automatically in your system")
}

SOURCES += \\
    %{MainCppName} \\
    %{TestCaseFileWithCppSuffix}
@endif
@if "%{TestFrameWork}" == "Catch2" || "%{TestFrameWork}" == "Catch2_dyn"
TEMPLATE = app
@if "%{Catch2NeedsQt}" == "true"
QT += gui
@else
CONFIG -= qt
CONFIG -= app_bundle
CONFIG += console
@endif

CONFIG += c++11
@endif
@if "%{TestFrameWork}" == "Catch2"
isEmpty(CATCH_INCLUDE_DIR): CATCH_INCLUDE_DIR=$$(CATCH_INCLUDE_DIR)
@if "%{CatchIncDir}" != ""
# set by Qt Creator wizard
isEmpty(CATCH_INCLUDE_DIR): CATCH_INCLUDE_DIR="%{CatchIncDir}"
@endif
!isEmpty(CATCH_INCLUDE_DIR): INCLUDEPATH *= $${CATCH_INCLUDE_DIR}

isEmpty(CATCH_INCLUDE_DIR): {
    message("CATCH_INCLUDE_DIR is not set, assuming Catch2 can be found automatically in your system")
}

SOURCES += \\
    %{MainCppName} \\
    %{TestCaseFileWithCppSuffix}
@endif
@if "%{TestFrameWork}" == "Catch2_dyn"
@if "%{Catch2Main}" == "true"
SOURCES = %{TestCaseFileWithCppSuffix} \\
        %{MainCppName}

CATCH2_MAIN=0

@else
SOURCES = %{TestCaseFileWithCppSuffix}

CATCH2_MAIN=1

@endif
include(catch-common.pri)
@endif
