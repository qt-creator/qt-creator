QT += testlib
@if "%RequireGUI%" == "false"
QT -= gui

CONFIG += qt console warn_on depend_includepath testcase
CONFIG -= app_bundle
@else
QT += gui
CONFIG += qt warn_on depend_includepath testcase
@endif

TEMPLATE = app

SOURCES += tst_%TestCaseName:l%.%CppSourceSuffix%
