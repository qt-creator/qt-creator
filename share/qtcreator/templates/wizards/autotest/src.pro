@if "%RequireGUI%" == "true"
QT += core gui
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets
@else
QT -= gui
CONFIG += console
CONFIG -= app_bundle
@endif

TEMPLATE = app
TARGET = %ProjectName%

SOURCES += main.%CppSourceSuffix%
