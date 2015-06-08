include(../../qtcreator.pri)
include(../shared/qtsingleapplication/qtsingleapplication.pri)

TEMPLATE = app
CONFIG += qtc_runnable
TARGET = $$IDE_APP_TARGET
DESTDIR = $$IDE_APP_PATH
QT -= testlib

HEADERS += ../tools/qtcreatorcrashhandler/crashhandlersetup.h
SOURCES += main.cpp ../tools/qtcreatorcrashhandler/crashhandlersetup.cpp

include(../rpath.pri)

LIBS *= -l$$qtLibraryName(ExtensionSystem) -l$$qtLibraryName(Aggregation) -l$$qtLibraryName(Utils)

QT_BREAKPAD_ROOT_PATH = $$(QT_BREAKPAD_ROOT_PATH)
!isEmpty(QT_BREAKPAD_ROOT_PATH) {
    include($$QT_BREAKPAD_ROOT_PATH/qtbreakpad.pri)
}
win32 {
    RC_FILE = qtcreator.rc
    target.path = $$QTC_PREFIX/bin
    INSTALLS += target
} else:macx {
    LIBS += -framework CoreFoundation
    ICON = qtcreator.icns
    FILETYPES.files = profile.icns prifile.icns
    FILETYPES.path = Contents/Resources
    QMAKE_BUNDLE_DATA += FILETYPES
    info.input = Info.plist.in
    info.output = $$IDE_BIN_PATH/../Info.plist
    QMAKE_SUBSTITUTES = info
} else {
    target.path  = $$QTC_PREFIX/bin
    INSTALLS    += target
}

DISTFILES += qtcreator.rc \
    Info.plist.in \
    $$PWD/app_version.h.in

QMAKE_SUBSTITUTES += $$PWD/app_version.h.in

CONFIG += no_batch
