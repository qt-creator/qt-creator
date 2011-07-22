TARGET = QtTest
TEMPLATE = lib
DEFINES += QTCREATOR_QTEST
DEFINES += QTTEST_PLUGIN_LEAN

include(../../qtcreatorplugin.pri)
include(../../plugins/projectexplorer/projectexplorer.pri)
include(../../plugins/texteditor/texteditor.pri)
include(../../plugins/coreplugin/coreplugin.pri)
include(../../plugins/debugger/debugger.pri)
include(../../plugins/qtsupport/qtsupport.pri)
include(../../plugins/remotelinux/remotelinux.pri)
include(../../plugins/qt4projectmanager/qt4projectmanager.pri)
include(../../libs/utils/utils.pri)
include(../../libs/qmljs/qmljs.pri)
include(../../libs/cplusplus/cplusplus.pri)

QT += network

FORMS += newtestfunctiondlg.ui \
    selectdlg.ui \
    recorddlg.ui \
    testsettingspropertiespage.ui \
    newtestcasedlg.ui \
    addmanualtestdlg.ui

HEADERS += qttestplugin.h \
    dialogs.h \
    newtestcasedlg.h \
    qsystem.h \
    resultsview.h \
    testselector.h \
    testcode.h \
    testconfigurations.h \
    testexecuter.h \
    testgenerator.h \
    testsettings.h \
    testsuite.h \
    testoutputwindow.h \
    testcontextmenu.h \
    testsettingspropertiespage.h \
    addmanualtestdlg.h \
    testresultuploader.h

SOURCES += qttestplugin.cpp \
    dialogs.cpp \
    newtestcasedlg.cpp \
    qsystem.cpp \
    resultsview.cpp \
    testselector.cpp \
    testcode.cpp \
    testconfigurations.cpp \
    testexecuter.cpp \
    testgenerator.cpp \
    testsettings.cpp \
    testoutputwindow.cpp \
    testcontextmenu.cpp \
    testsettingspropertiespage.cpp \
    addmanualtestdlg.cpp \
    testresultuploader.cpp

RESOURCES += qttest.qrc
