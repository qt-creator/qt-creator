include(../../qtcreatorplugin.pri)
include(callgrind/callgrind.pri)
include(xmlprotocol/xmlprotocol.pri)
QT *= network

CONFIG += exceptions

HEADERS += \
    valgrindplugin.h \
    valgrindengine.h \
    valgrindconfigwidget.h \
    valgrindsettings.h \
    valgrindrunner.h \
    callgrindcostdelegate.h \
    callgrindcostview.h \
    callgrindhelper.h \
    callgrindnamedelegate.h \
    callgrindtool.h \
    callgrindvisualisation.h \
    callgrindengine.h \
    callgrindtextmark.h \
    memchecktool.h \
    memcheckerrorview.h \
    suppressiondialog.h

SOURCES += \
    valgrindplugin.cpp \
    valgrindengine.cpp \
    valgrindconfigwidget.cpp \
    valgrindsettings.cpp \
    valgrindrunner.cpp \
    callgrindcostdelegate.cpp \
    callgrindcostview.cpp \
    callgrindhelper.cpp \
    callgrindnamedelegate.cpp \
    callgrindtool.cpp \
    callgrindvisualisation.cpp \
    callgrindengine.cpp \
    callgrindtextmark.cpp \
    memchecktool.cpp \
    memcheckerrorview.cpp \
    suppressiondialog.cpp

FORMS += \
    valgrindconfigwidget.ui

RESOURCES += \
    valgrind.qrc

equals(TEST, 1) {
    DEFINES += "PARSERTESTS_DATA_DIR=\\\"$$_PRO_FILE_PWD_/unit_testdata\\\""
    DEFINES += "VALGRIND_FAKE_PATH=\\\"$$IDE_BUILD_TREE/src/tools/valgrindfake\\\""
    DEFINES += "TESTRUNNER_SRC_DIR=\\\"$$_PRO_FILE_PWD_/../../../tests/auto/valgrind/memcheck/testapps\\\""
    DEFINES += "TESTRUNNER_APP_DIR=\\\"$(PWD)/../../../tests/auto/valgrind/memcheck/testapps\\\""

    HEADERS += valgrindmemcheckparsertest.h \
        valgrindtestrunnertest.h

    SOURCES += valgrindmemcheckparsertest.cpp \
        valgrindtestrunnertest.cpp
}
