QTC_LIB_DEPENDS += utils

isEmpty(IDE_SOURCE_TREE): IDE_SOURCE_TREE=$$(QTC_SOURCE)
isEmpty(IDE_BUILD_TREE): IDE_BUILD_TREE=$$(QTC_BUILD)
isEmpty(IDE_SOURCE_TREE): error(Set QTC_SOURCE environment variable)
isEmpty(IDE_BUILD_TREE): error(Set QTC_BUILD environment variable)
isEmpty(QTC_PLUGIN_DIRS): error(Set QTC_PLUGIN_DIRS environment variable for extra plugins)

include(../../../../qt-creator/qtcreator.pri)
include(../../../../qt-creator/tests/auto/qttestrpath.pri)

PLUGINDIR=$$PWD/../

QT       += testlib
QT       -= gui

CONFIG   += console
CONFIG   += testcase
CONFIG   -= app_bundle

TEMPLATE = app
