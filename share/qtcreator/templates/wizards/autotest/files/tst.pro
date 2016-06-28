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
@else
include(../gtest_dependency.pri)

TEMPLATE = app
@if "%{GTestCXX11}" == "true"
CONFIG += console c++11
@else
CONFIG += console
@endif
CONFIG -= app_bundle
CONFIG += thread
CONFIG -= qt

HEADERS += \
    %{TestCaseFileWithHeaderSuffix}

SOURCES += \
    %{MainCppName}
@endif
