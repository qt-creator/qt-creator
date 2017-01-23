include(qtcreator.pri)

#version check qt
!minQtVersion(5, 6, 0) {
    message("Cannot build Qt Creator with Qt version $${QT_VERSION}.")
    error("Use at least Qt 5.6.0.")
}

include(doc/doc.pri)

TEMPLATE  = subdirs
CONFIG   += ordered

SUBDIRS = src share
unix:!macx:!isEmpty(copydata):SUBDIRS += bin
!isEmpty(BUILD_TESTS):SUBDIRS += tests

DISTFILES += dist/copyright_template.txt \
    README.md \
    $$files(dist/changes-*) \
    qtcreator.qbs \
    qbs/pluginjson/pluginjson.qbs \
    $$files(dist/installer/ifw/config/config-*) \
    dist/installer/ifw/packages/org.qtproject.qtcreator/meta/package.xml.in \
    dist/installer/ifw/packages/org.qtproject.qtcreator.application/meta/installscript.qs \
    dist/installer/ifw/packages/org.qtproject.qtcreator.application/meta/package.xml.in \
    dist/installer/ifw/packages/org.qtproject.qtcreator.application/meta/license.txt \
    $$files(scripts/*.py) \
    $$files(scripts/*.sh) \
    $$files(scripts/*.pl)

exists(src/shared/qbs/qbs.pro) {
    # Make sure the qbs dll ends up alongside the Creator executable.
    QBS_DLLDESTDIR = $${IDE_BUILD_TREE}/bin
    cache(QBS_DLLDESTDIR)
    QBS_DESTDIR = $${IDE_LIBRARY_PATH}
    cache(QBS_DESTDIR)
    QBSLIBDIR = $${IDE_LIBRARY_PATH}
    cache(QBSLIBDIR)
    QBS_INSTALL_PREFIX = $${QTC_PREFIX}
    cache(QBS_INSTALL_PREFIX)
    QBS_LIB_INSTALL_DIR = $$INSTALL_LIBRARY_PATH
    cache(QBS_LIB_INSTALL_DIR)
    QBS_RESOURCES_BUILD_DIR = $${IDE_DATA_PATH}/qbs
    cache(QBS_RESOURCES_BUILD_DIR)
    QBS_RESOURCES_INSTALL_DIR = $$INSTALL_DATA_PATH/qbs
    cache(QBS_RESOURCES_INSTALL_DIR)
    macx {
        QBS_PLUGINS_BUILD_DIR = $${IDE_PLUGIN_PATH}
        QBS_APPS_RPATH_DIR = @loader_path/../Frameworks
    } else {
        QBS_PLUGINS_BUILD_DIR = $$IDE_PLUGIN_PATH
        QBS_APPS_RPATH_DIR = \$\$ORIGIN/../$$IDE_LIBRARY_BASENAME/qtcreator
    }
    cache(QBS_PLUGINS_BUILD_DIR)
    cache(QBS_APPS_RPATH_DIR)
    QBS_PLUGINS_INSTALL_DIR = $$INSTALL_PLUGIN_PATH
    cache(QBS_PLUGINS_INSTALL_DIR)
    QBS_LIBRARY_DIRNAME = $${IDE_LIBRARY_BASENAME}
    cache(QBS_LIBRARY_DIRNAME)
    QBS_APPS_DESTDIR = $${IDE_BIN_PATH}
    cache(QBS_APPS_DESTDIR)
    QBS_APPS_INSTALL_DIR = $$INSTALL_BIN_PATH
    cache(QBS_APPS_INSTALL_DIR)
    QBS_LIBEXEC_DESTDIR = $${IDE_LIBEXEC_PATH}
    cache(QBS_LIBEXEC_DESTDIR)
    QBS_LIBEXEC_INSTALL_DIR = $$INSTALL_LIBEXEC_PATH
    cache(QBS_LIBEXEC_INSTALL_DIR)
    QBS_RELATIVE_LIBEXEC_PATH = $$relative_path($$QBS_LIBEXEC_DESTDIR, $$QBS_APPS_DESTDIR)
    isEmpty(QBS_RELATIVE_LIBEXEC_PATH):QBS_RELATIVE_LIBEXEC_PATH = .
    cache(QBS_RELATIVE_LIBEXEC_PATH)
    QBS_RELATIVE_PLUGINS_PATH = $$relative_path($$QBS_PLUGINS_BUILD_DIR, $$QBS_APPS_DESTDIR$$)
    cache(QBS_RELATIVE_PLUGINS_PATH)
    QBS_RELATIVE_SEARCH_PATH = $$relative_path($$QBS_RESOURCES_BUILD_DIR, $$QBS_APPS_DESTDIR)
    cache(QBS_RELATIVE_SEARCH_PATH)
    !qbs_no_dev_install {
        QBS_CONFIG_ADDITION = qbs_no_dev_install qbs_enable_project_file_updates
        cache(CONFIG, add, QBS_CONFIG_ADDITION)
    }

    # Create qbs documentation targets.
    QBS_DOCS_BUILD_DIR=$$IDE_DOC_PATH
    QBS_HTML_DOC_PATH=$$OUT_PWD/doc/html-qbs
    QBS_DOCS_INSTALL_DIR=$$INSTALL_DOC_PATH
    include(src/shared/qbs/qbs_version.pri)
    include(src/shared/qbs/doc/doc_targets.pri)
    docs.depends += qbs_docs
    install_docs.depends += install_inst_qbs_qch_docs
}

contains(QT_ARCH, i386): ARCHITECTURE = x86
else: ARCHITECTURE = $$QT_ARCH

macx: PLATFORM = "mac"
else:win32: PLATFORM = "windows"
else:linux-*: PLATFORM = "linux-$${ARCHITECTURE}"
else: PLATFORM = "unknown"

BASENAME = $$(INSTALL_BASENAME)
isEmpty(BASENAME): BASENAME = qt-creator-$${PLATFORM}$(INSTALL_EDITION)-$${QTCREATOR_VERSION}$(INSTALL_POSTFIX)

macx:INSTALLER_NAME = "qt-creator-$${QTCREATOR_VERSION}"
else:INSTALLER_NAME = "$${BASENAME}"

linux {
    appstream.files = dist/org.qt-project.qtcreator.appdata.xml
    appstream.path = $$QTC_PREFIX/share/metainfo/

    desktop.files = dist/org.qt-project.qtcreator.desktop
    desktop.path = $$QTC_PREFIX/share/applications/

    INSTALLS += appstream desktop
}

macx {
    APPBUNDLE = "$$OUT_PWD/bin/Qt Creator.app"
    BINDIST_SOURCE = "$$OUT_PWD/bin/Qt Creator.app"
    BINDIST_INSTALLER_SOURCE = $$BINDIST_SOURCE
    deployqt.commands = $$PWD/scripts/deployqtHelper_mac.sh \"$${APPBUNDLE}\" \"$$[QT_INSTALL_BINS]\" \"$$[QT_INSTALL_TRANSLATIONS]\" \"$$[QT_INSTALL_PLUGINS]\" \"$$[QT_INSTALL_IMPORTS]\" \"$$[QT_INSTALL_QML]\"
    codesign.commands = codesign --deep -s \"$(SIGNING_IDENTITY)\" $(SIGNING_FLAGS) \"$${APPBUNDLE}\"
    dmg.commands = $$PWD/scripts/makedmg.sh $$OUT_PWD/bin $${BASENAME}.dmg
    #dmg.depends = deployqt
    QMAKE_EXTRA_TARGETS += codesign dmg
} else {
    BINDIST_SOURCE = "$(INSTALL_ROOT)$$QTC_PREFIX"
    BINDIST_INSTALLER_SOURCE = "$$BINDIST_SOURCE/*"
    deployqt.commands = python -u $$PWD/scripts/deployqt.py -i \"$(INSTALL_ROOT)$$QTC_PREFIX\" \"$(QMAKE)\"
    deployqt.depends = install
    win32 {
        deployartifacts.depends = install
        deployartifacts.commands = git clone --depth 1 -b $$BINARY_ARTIFACTS_BRANCH \
                "http://code.qt.io/qt-creator/binary-artifacts.git" \
            && xcopy /s /q /y /i "binary-artifacts\\win32" \"$(INSTALL_ROOT)$$QTC_PREFIX\" \
            && rmdir /s /q binary-artifacts
        QMAKE_EXTRA_TARGETS += deployartifacts
    }
}

INSTALLER_ARCHIVE_FROM_ENV = $$(INSTALLER_ARCHIVE)
isEmpty(INSTALLER_ARCHIVE_FROM_ENV) {
    INSTALLER_ARCHIVE = $$OUT_PWD/$${BASENAME}-installer-archive.7z
} else {
    INSTALLER_ARCHIVE = $$OUT_PWD/$$(INSTALLER_ARCHIVE)
}

#bindist.depends = deployqt
bindist.commands = 7z a -mx9 $$OUT_PWD/$${BASENAME}.7z \"$$BINDIST_SOURCE\"
#bindist_installer.depends = deployqt
bindist_installer.commands = 7z a -mx9 $${INSTALLER_ARCHIVE} \"$$BINDIST_INSTALLER_SOURCE\"
installer.depends = bindist_installer
installer.commands = python -u $$PWD/scripts/packageIfw.py -i \"$(IFW_PATH)\" -v $${QTCREATOR_VERSION} -a \"$${INSTALLER_ARCHIVE}\" "$$INSTALLER_NAME"

macx {
    codesign_installer.commands = codesign -s \"$(SIGNING_IDENTITY)\" $(SIGNING_FLAGS) \"$${INSTALLER_NAME}.app\"
    dmg_installer.commands = hdiutil create -srcfolder "$${INSTALLER_NAME}.app" -volname \"Qt Creator\" -format UDBZ "$${BASENAME}-installer.dmg" -ov -scrub -size 1g -verbose
    QMAKE_EXTRA_TARGETS += codesign_installer dmg_installer
}

win32 {
    deployqt.commands ~= s,/,\\\\,g
    bindist.commands ~= s,/,\\\\,g
    bindist_installer.commands ~= s,/,\\\\,g
    installer.commands ~= s,/,\\\\,g
}

QMAKE_EXTRA_TARGETS += deployqt bindist bindist_installer installer
