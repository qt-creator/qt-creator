include(qtcreator.pri)

#version check qt
!minQtVersion(4, 8, 0) {
    message("Cannot build Qt Creator with Qt version $${QT_VERSION}.")
    error("Use at least Qt 4.8.0.")
}

include(doc/doc.pri)

TEMPLATE  = subdirs
CONFIG   += ordered

SUBDIRS = src share
unix:!macx:!isEmpty(copydata):SUBDIRS += bin
!isEmpty(BUILD_TESTS):SUBDIRS += tests

OTHER_FILES += dist/copyright_template.txt \
    $$files(dist/changes-*) \
    qtcreator.qbs \
    qbs/pluginspec/pluginspec.qbs \
    $$files(dist/installer/ifw/config/config-*) \
    dist/installer/ifw/packages/org.qtproject.qtcreator/meta/package.xml.in \
    dist/installer/ifw/packages/org.qtproject.qtcreator.application/meta/installscript.qs \
    dist/installer/ifw/packages/org.qtproject.qtcreator.application/meta/package.xml.in \
    dist/installer/ifw/packages/org.qtproject.qtcreator.application/meta/license.txt \
    $$files(scripts/*.py) \
    $$files(scripts/*.sh) \
    $$files(scripts/*.pl)

qmake_cache = $$targetPath($$IDE_BUILD_TREE/.qmake.cache)
!equals(QMAKE_HOST.os, Windows) {
    maybe_quote = "\""
    maybe_backslash = "\\"
}
system("echo $${maybe_quote}$${LITERAL_HASH} config for qmake$${maybe_quote} > $$qmake_cache")
# Make sure the qbs dll ends up alongside the Creator executable.
exists(src/shared/qbs/qbs.pro) {
    system("echo QBS_DLLDESTDIR = $${IDE_BUILD_TREE}/bin >> $$qmake_cache")
    system("echo QBS_DESTDIR = $${maybe_backslash}\"$${IDE_LIBRARY_PATH}$${maybe_backslash}\" >> $$qmake_cache")
    system("echo QBSLIBDIR = $${maybe_backslash}\"$${IDE_LIBRARY_PATH}$${maybe_backslash}\" >> $$qmake_cache")
    system("echo QBS_INSTALL_PREFIX = $${QTC_PREFIX} >> $$qmake_cache")
    system("echo QBS_LIB_INSTALL_DIR = $${QTC_PREFIX}/$${IDE_LIBRARY_BASENAME}/qtcreator >> $$qmake_cache")
    system("echo QBS_RESOURCES_BUILD_DIR = $${maybe_backslash}\"$${IDE_DATA_PATH}/qbs$${maybe_backslash}\" >> $$qmake_cache")
    system("echo QBS_RESOURCES_INSTALL_DIR = $${QTC_PREFIX}/share/qtcreator/qbs >> $$qmake_cache")
    system("echo CONFIG += qbs_no_dev_install >> $$qmake_cache")
}

_QMAKE_CACHE_ = $$qmake_cache # Qt 4 support prevents us from using cache(), so tell Qt 5 about the cache

contains(QT_ARCH, i386): ARCHITECTURE = x86
else: ARCHITECTURE = $$QT_ARCH

macx: PLATFORM = "mac"
else:win32: PLATFORM = "windows"
else:linux-*: PLATFORM = "linux-$${ARCHITECTURE}"
else: PLATFORM = "unknown"

PATTERN = $${PLATFORM}$(INSTALL_EDITION)-$${QTCREATOR_VERSION}$(INSTALL_POSTFIX)

macx:INSTALLER_NAME = "qt-creator-$${QTCREATOR_VERSION}"
else:INSTALLER_NAME = "qt-creator-$${PATTERN}"

macx {
    APPBUNDLE = "$$OUT_PWD/bin/Qt Creator.app"
    BINDIST_SOURCE = "$$OUT_PWD/bin/Qt Creator.app"
    BINDIST_INSTALLER_SOURCE = $$BINDIST_SOURCE
    deployqt.commands = $$PWD/scripts/deployqtHelper_mac.sh \"$${APPBUNDLE}\" \"$$[QT_INSTALL_TRANSLATIONS]\" \"$$[QT_INSTALL_PLUGINS]\" \"$$[QT_INSTALL_IMPORTS]\" \"$$[QT_INSTALL_QML]\"
    codesign.commands = codesign -s \"$(SIGNING_IDENTITY)\" $(SIGNING_FLAGS) \"$${APPBUNDLE}\"
    dmg.commands = $$PWD/scripts/makedmg.sh $$OUT_PWD/bin qt-creator-$${PATTERN}.dmg
    dmg.depends = deployqt
    QMAKE_EXTRA_TARGETS += codesign dmg
} else {
    BINDIST_SOURCE = "$(INSTALL_ROOT)$$QTC_PREFIX"
    BINDIST_INSTALLER_SOURCE = "$$BINDIST_SOURCE/*"
    deployqt.commands = python -u $$PWD/scripts/deployqt.py -i \"$(INSTALL_ROOT)$$QTC_PREFIX\"
    deployqt.depends = install
    win32 {
        deployartifacts.depends = install
        deployartifacts.commands = git clone "git://gitorious.org/qt-creator/binary-artifacts.git" -b $$BINARY_ARTIFACTS_BRANCH&& xcopy /s /q /y /i "binary-artifacts\\win32" \"$(INSTALL_ROOT)$$QTC_PREFIX\"&& rmdir /s /q binary-artifacts
        QMAKE_EXTRA_TARGETS += deployartifacts
    }
}

INSTALLER_ARCHIVE_FROM_ENV = $$(INSTALLER_ARCHIVE)
isEmpty(INSTALLER_ARCHIVE_FROM_ENV) {
    INSTALLER_ARCHIVE = $$OUT_PWD/qt-creator-$${PATTERN}-installer-archive.7z
} else {
    INSTALLER_ARCHIVE = $$OUT_PWD/$$(INSTALLER_ARCHIVE)
}

bindist.depends = deployqt
bindist.commands = 7z a -mx9 $$OUT_PWD/qt-creator-$${PATTERN}.7z \"$$BINDIST_SOURCE\"
bindist_installer.depends = deployqt
bindist_installer.commands = 7z a -mx9 $${INSTALLER_ARCHIVE} \"$$BINDIST_INSTALLER_SOURCE\"
installer.depends = bindist_installer
installer.commands = python -u $$PWD/scripts/packageIfw.py -i \"$(IFW_PATH)\" -v $${QTCREATOR_VERSION} -a \"$${INSTALLER_ARCHIVE}\" "$$INSTALLER_NAME"

macx {
    # this should be very temporary:
    MENU_NIB = $$(MENU_NIB_FILE)
    isEmpty(MENU_NIB): MENU_NIB = "FATAT_SET_MENU_NIB_FILE_ENV"
    copy_menu_nib_installer.commands = cp -R \"$$MENU_NIB\" \"$${INSTALLER_NAME}.app/Contents/Resources\"

    codesign_installer.commands = codesign -s \"$(SIGNING_IDENTITY)\" $(SIGNING_FLAGS) \"$${INSTALLER_NAME}.app\"
    dmg_installer.commands = hdiutil create -srcfolder "$${INSTALLER_NAME}.app" -volname \"Qt Creator\" -format UDBZ "qt-creator-$${PATTERN}-installer.dmg" -ov -scrub -stretch 2g
    QMAKE_EXTRA_TARGETS += codesign_installer dmg_installer copy_menu_nib_installer
}

win32 {
    deployqt.commands ~= s,/,\\\\,g
    bindist.commands ~= s,/,\\\\,g
    bindist_installer.commands ~= s,/,\\\\,g
    installer.commands ~= s,/,\\\\,g
}

QMAKE_EXTRA_TARGETS += deployqt bindist bindist_installer installer
