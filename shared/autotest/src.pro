@if "%RequireGUI%" == "true"
QT += gui widgets
@else
QT += console
CONFIG -= app_bundle
@endif

TEMPLATE = app

SOURCES += main.%CppSourceSuffix%
