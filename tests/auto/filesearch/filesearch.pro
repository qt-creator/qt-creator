include(../qttest.pri)

include($$IDE_SOURCE_TREE/src/libs/utils/utils.pri)

INCLUDEPATH += $$IDE_SOURCE_TREE/src/libs/utils
# Input
SOURCES += tst_filesearch.cpp

OTHER_FILES += testfile.txt

RESOURCES += tst_filesearch.qrc
