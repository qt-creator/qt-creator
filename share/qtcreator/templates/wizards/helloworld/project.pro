QT       = core
%NETWORK%QT += network
@if "%SCRIPT%" == "true"
QT += script
@endif
CONFIG   += console c++11
CONFIG   -= app_bundle

TEMPLATE = app
SOURCES += main.cpp
