TEMPLATE = app
TARGET = testdata_renameheaders
INCLUDEPATH += subdir2
CONFIG += no_include_pwd

HEADERS = header.h subdir1/header1.h subdir2/header2.h
SOURCES = main.cpp subdir1/file1.cpp subdir2/file2.cpp
