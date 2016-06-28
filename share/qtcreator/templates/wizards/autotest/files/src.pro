@if "%{TestFrameWork}" == "QtTest"
@if "%{RequireGUI}" == "true"
QT += core gui
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets
@else
QT -= gui
@endif
@else
CONFIG -= qt
@endif
CONFIG += console
CONFIG -= app_bundle

TEMPLATE = app

TARGET = %{ProjectName}

SOURCES += %{MainCppName}
