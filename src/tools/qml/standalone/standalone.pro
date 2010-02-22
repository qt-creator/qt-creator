!contains(QT_CONFIG, declarative) {
    error("Qt is not configured with the declarative model.");
}

TEMPLATE = app
CONFIG(debug, debug|release):CONFIG += console
TARGET = bauhaus
macx:TARGET = Bauhaus
CONFIG += qt
CONFIG += webkit
QT += gui opengl

HEADERS += \
    aboutdialog.h \
    mainwindow.h \
    application.h \
    welcomescreen.h

SOURCES += \
    aboutdialog.cpp \
    main.cpp \
    mainwindow.cpp \
    application.cpp \
    welcomescreen.cpp

include(../../../libs/qmljs/qmljs-lib.pri)
HEADERS+=../../../libs/utils/changeset.h
SOURCES+=../../../libs/utils/changeset.cpp
INCLUDEPATH+=../../../libs
DEFINES+=QTCREATOR_UTILS_STATIC_LIB QML_BUILD_STATIC_LIB

include(../../../plugins/qmldesigner/config.pri)
include(../../../plugins/qmldesigner/components/integration/integration.pri)
include(../../../plugins/qmldesigner/components/propertyeditor/propertyeditor.pri)
include(../../../plugins/qmldesigner/components/formeditor/formeditor.pri)
include(../../../plugins/qmldesigner/components/navigator/navigator.pri)
include(../../../plugins/qmldesigner/components/stateseditor/stateseditor.pri)
include(../../../plugins/qmldesigner/components/itemlibrary/itemlibrary.pri)
include(../../../plugins/qmldesigner/components/resources/resources.pri)
include(../../../plugins/qmldesigner/components/pluginmanager/pluginmanager.pri)
include(../../../plugins/qmldesigner/components/themeloader/qts60stylethemeio.pri)
include (../../../plugins/qmldesigner/core/core.pri)
RESOURCES += bauhaus.qrc
win32:RC_FILE = bauhaus.rc
macx {
    ICON = bauhaus-logo.icns
    QMAKE_INFO_PLIST = Info.plist
}
QMAKE_CXXFLAGS_HIDESYMS=""

