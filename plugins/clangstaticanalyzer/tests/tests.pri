QTC_LIB_DEPENDS += utils

isEmpty(IDE_SOURCE_TREE): IDE_SOURCE_TREE=$$(QTC_SOURCE)
isEmpty(IDE_BUILD_TREE): IDE_BUILD_TREE=$$(QTC_BUILD)
isEmpty(IDE_SOURCE_TREE): error(Set QTC_SOURCE environment variable)
isEmpty(IDE_BUILD_TREE): error(Set QTC_BUILD environment variable)

PLUGINDIR = $$PWD/..
INCLUDEPATH += $$PLUGINDIR/..

include($$IDE_SOURCE_TREE/qtcreator.pri)
include($$IDE_SOURCE_TREE/tests/auto/qttestrpath.pri)

QT       += testlib
QT       -= gui

CONFIG   += console
CONFIG   += testcase
CONFIG   -= app_bundle

TEMPLATE = app
