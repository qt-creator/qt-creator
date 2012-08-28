include(qtcreator.pri)

#version check qt
!minQtVersion(4, 7, 4) {
    message("Cannot build Qt Creator with Qt version $${QT_VERSION}.")
    error("Use at least Qt 4.7.4.")
}

include(doc/doc.pri)

TEMPLATE  = subdirs
CONFIG   += ordered

SUBDIRS = src share lib/qtcreator/qtcomponents
unix:!macx:!isEmpty(copydata):SUBDIRS += bin

OTHER_FILES += dist/copyright_template.txt \
    $$files(dist/changes-*)

macx {
    APPBUNDLE = "$$OUT_PWD/bin/Qt Creator.app"
    deployqt.commands = $$PWD/scripts/deployqtHelper_mac.sh \"$${APPBUNDLE}\"
    bindist.commands = 7z a -mx9 $$OUT_PWD/qt-creator-mac$(INSTALL_EDITION)-$${QTCREATOR_VERSION}$(INSTALL_POSTFIX).7z \"$$OUT_PWD/bin/Qt Creator.app/\"
    dmg.commands = $$PWD/scripts/makedmg.sh $$OUT_PWD/bin qt-creator-mac$(INSTALL_EDITION)-$${QTCREATOR_VERSION}$(INSTALL_POSTFIX).dmg
    dmg.depends = deployqt
    QMAKE_EXTRA_TARGETS += dmg
} else {
    deployqt.commands = $$PWD/scripts/deployqt.py -i $(INSTALL_ROOT)
    deployqt.depends = install
    win32 {
        bindist.commands ~= s,/,\\\\,g
        deployqt.commands ~= s,/,\\\\,g
        deployartifacts.depends = install
        PLATFORM="windows"
        deployartifacts.commands = git clone "git://gitorious.org/qt-creator/binary-artifacts.git"&& xcopy /s /q /y /i "binary-artifacts\\win32" $(INSTALL_ROOT)&& rmdir /s /q binary-artifacts
        QMAKE_EXTRA_TARGETS += deployartifacts
    }
    else:linux-*:PLATFORM="linux-$${QT_ARCH}"
    else:PLATFORM="unknown"
    bindist.commands = $$PWD/scripts/bindistHelper.py "$(INSTALL_ROOT)" "$${PLATFORM}$(INSTALL_EDITION)-$${QTCREATOR_VERSION}$(INSTALL_POSTFIX)"
}
bindist.depends = deployqt
QMAKE_EXTRA_TARGETS += deployqt bindist
