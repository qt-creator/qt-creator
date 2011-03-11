QT = core gui
macx:CONFIG -= app_bundle
TEMPLATE = app
TARGET = generate-ast
INCLUDEPATH += . ../../libs

include(../../libs/cplusplus/cplusplus-lib.pri)

# Input
SOURCES += generate-ast.cpp ../../libs/utils/changeset.cpp
