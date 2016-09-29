include(../../qtcreator.pri)
include(../shared/qtsingleapplication/qtsingleapplication.pri)

TEMPLATE = app
CONFIG += qtc_runnable sliced_bundle
TARGET = $$IDE_APP_TARGET
DESTDIR = $$IDE_APP_PATH
VERSION = $$QTCREATOR_VERSION
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
} else:macx {
    LIBS += -framework CoreFoundation
    minQtVersion(5, 7, 1) {
        QMAKE_ASSET_CATALOGS = $$PWD/qtcreator.xcassets
        QMAKE_ASSET_CATALOGS_BUILD_PATH = $$IDE_DATA_PATH
        QMAKE_ASSET_CATALOGS_INSTALL_PATH = $$INSTALL_DATA_PATH
        QMAKE_ASSET_CATALOGS_APP_ICON = qtcreator
    } else {
        ASSETCATALOG.files = $$PWD/qtcreator.xcassets
        macx-xcode {
            QMAKE_BUNDLE_DATA += ASSETCATALOG
        } else {
            ASSETCATALOG.output = $$IDE_DATA_PATH/qtcreator.icns
            ASSETCATALOG.commands = xcrun actool \
                --app-icon qtcreator \
                --output-partial-info-plist $$shell_quote($(TMPDIR)/qtcreator.Info.plist) \
                --platform macosx \
                --minimum-deployment-target 10.7 \
                --compile $$shell_quote($$IDE_DATA_PATH) \
                $$shell_quote($$PWD/qtcreator.xcassets) > /dev/null
            ASSETCATALOG.input = ASSETCATALOG.files
            ASSETCATALOG.CONFIG += no_link target_predeps
            QMAKE_EXTRA_COMPILERS += ASSETCATALOG
            icns.files = \
                $$IDE_DATA_PATH/qtcreator.icns \
                $$IDE_DATA_PATH/prifile.icns \
                $$IDE_DATA_PATH/profile.icns
            icns.path = $$INSTALL_DATA_PATH
            icns.CONFIG += no_check_exist
            INSTALLS += icns
        }
    }
    QMAKE_INFO_PLIST = Info.plist
}

target.path = $$INSTALL_APP_PATH
INSTALLS += target

DISTFILES += qtcreator.rc \
    Info.plist \
    $$PWD/app_version.h.in

QMAKE_SUBSTITUTES += $$PWD/app_version.h.in

CONFIG += no_batch
