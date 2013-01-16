include(qtcreator.pri)

#version check qt
!minQtVersion(4, 8, 0) {
    message("Cannot build Qt Creator with Qt version $${QT_VERSION}.")
    error("Use at least Qt 4.8.0.")
}

include(doc/doc.pri)

TEMPLATE  = subdirs
CONFIG   += ordered

SUBDIRS = src share lib/qtcreator/qtcomponents
unix:!macx:!isEmpty(copydata):SUBDIRS += bin

OTHER_FILES += dist/copyright_template.txt \
    $$files(dist/changes-*) \
    qtcreator.qbp \
    qbs/pluginspec/pluginspec.qbs

contains(QT_ARCH, i386): ARCHITECTURE = x86
else: ARCHITECTURE = $$QT_ARCH

macx: PLATFORM = "mac"
else:win32: PLATFORM = "windows"
else:linux-*: PLATFORM = "linux-$${ARCHITECTURE}"
else: PLATFORM = "unknown"

PATTERN = $${PLATFORM}$(INSTALL_EDITION)-$${QTCREATOR_VERSION}$(INSTALL_POSTFIX)

macx {
    APPBUNDLE = "$$OUT_PWD/bin/Qt Creator.app"
    BINDIST_SOURCE = "$$OUT_PWD/bin/Qt Creator.app"
    BINDIST_INSTALLER_SOURCE = $$BINDIST_SOURCE
    deployqt.commands = $$PWD/scripts/deployqtHelper_mac.sh \"$${APPBUNDLE}\" \"$$[QT_INSTALL_TRANSLATIONS]\" \"$$[QT_INSTALL_PLUGINS]\"
    codesign.commands = codesign -s \"$(SIGNING_IDENTITY)\" $(SIGNING_FLAGS) \"$${APPBUNDLE}\"
    dmg.commands = $$PWD/scripts/makedmg.sh $$OUT_PWD/bin qt-creator-$${PATTERN}.dmg
    dmg.depends = deployqt
    QMAKE_EXTRA_TARGETS += codesign dmg
} else {
    BINDIST_SOURCE = "$(INSTALL_ROOT)$$QTC_PREFIX"
    BINDIST_INSTALLER_SOURCE = "$$BINDIST_SOURCE/*"
    deployqt.commands = $$PWD/scripts/deployqt.py -i \"$(INSTALL_ROOT)$$QTC_PREFIX\"
    deployqt.depends = install
    win32 {
        deployartifacts.depends = install
        deployartifacts.commands = git clone "git://gitorious.org/qt-creator/binary-artifacts.git"&& xcopy /s /q /y /i "binary-artifacts\\win32" \"$(INSTALL_ROOT)$$QTC_PREFIX\"&& rmdir /s /q binary-artifacts
        QMAKE_EXTRA_TARGETS += deployartifacts
    }
}

INSTALLER_ARCHIVE = $$OUT_PWD/qt-creator-$${PATTERN}-installer-archive.7z

bindist.depends = deployqt
bindist.commands = 7z a -mx9 $$OUT_PWD/qt-creator-$${PATTERN}.7z \"$$BINDIST_SOURCE\"
bindist_installer.depends = deployqt
bindist_installer.commands = 7z a -mx9 $$OUT_PWD/qt-creator-$${PATTERN}-installer-archive.7z \"$$BINDIST_INSTALLER_SOURCE\"
installer.depends = bindist_installer
installer.commands = $$PWD/scripts/packageIfw.py -i \"$(IFW_PATH)\" -v $${QTCREATOR_VERSION} -a \"$$INSTALLER_ARCHIVE\" "qt-creator-$${PATTERN}"

win32 {
    deployqt.commands ~= s,/,\\\\,g
    bindist.commands ~= s,/,\\\\,g
    bindist_installer.commands ~= s,/,\\\\,g
    installer.commands ~= s,/,\\\\,g
}

QMAKE_EXTRA_TARGETS += deployqt bindist bindist_installer installer
