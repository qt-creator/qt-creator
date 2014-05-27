TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += main.%CppSourceSuffix%

include(deployment.pri)
qtcAddDeployment()
