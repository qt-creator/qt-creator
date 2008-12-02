TEMPLATE = app

macx:CONFIG-=app_bundle

# Input
HEADERS += plugindialog.h
SOURCES += plugindialog.cpp

include(../../extensionsystem_test.pri)
