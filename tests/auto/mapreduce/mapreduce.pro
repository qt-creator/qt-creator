QTC_LIB_DEPENDS += utils
include(../qttest.pri)

# Input
SOURCES += tst_mapreduce.cpp
OTHER_FILES += $$IDE_SOURCE_TREE/src/libs/utils/mapreduce.h
