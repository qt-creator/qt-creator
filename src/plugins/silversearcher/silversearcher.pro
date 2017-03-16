include(../../qtcreatorplugin.pri)

SOURCES += \
    findinfilessilversearcher.cpp \
    silversearcheroutputparser.cpp \
    silversearcherplugin.cpp

HEADERS += \
    findinfilessilversearcher.h \
    silversearcheroutputparser.h \
    silversearcherplugin.h

equals(TEST, 1) {
  SOURCES += outputparser_test.cpp
  HEADERS += outputparser_test.h
}
