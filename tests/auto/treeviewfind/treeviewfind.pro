include(../qttest.pri)

include($$IDE_SOURCE_TREE/src/plugins/find/find.pri)

LIBS *= -L$$IDE_LIBRARY_PATH/QtProject

SOURCES += \
    tst_treeviewfind.cpp
