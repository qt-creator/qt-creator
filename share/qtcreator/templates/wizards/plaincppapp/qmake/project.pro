TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += main.%CppSourceSuffix%

include(deployment.pri)
qtcAddDeployment()
