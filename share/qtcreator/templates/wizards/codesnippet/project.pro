TEMPLATE = app
@if "%TYPE%" == "core"
QT = core
@else
QT = core gui
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets
@endif
@if "%CONSOLE%" == "true"
CONFIG += console
@endif
@if "%APP_BUNDLE%" == "false"
CONFIG -= app_bundle
@endif
CONFIG += c++11

SOURCES += \\
        main.%CppSourceSuffix%
