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
    $$files(dist/changes-*)

macx: PLATFORM = "mac"
else:win32: PLATFORM = "windows"
else:linux-*: PLATFORM = "linux-$${QT_ARCH}"
else: PLATFORM = "unknown"

PATTERN = $${PLATFORM}$(INSTALL_EDITION)-$${QTCREATOR_VERSION}$(INSTALL_POSTFIX)

macx {
    APPBUNDLE = "$$OUT_PWD/bin/Qt Creator.app"
    BINDIST_SOURCE = "$$OUT_PWD/bin/Qt Creator.app"
    deployqt.commands = $$PWD/scripts/deployqtHelper_mac.sh \"$${APPBUNDLE}\"
    codesign.commands = codesign -s \"$(SIGNING_IDENTITY)\" \"$${APPBUNDLE}\"
    dmg.commands = $$PWD/scripts/makedmg.sh $$OUT_PWD/bin qt-creator-$${PATTERN}.dmg
    dmg.depends = deployqt
    QMAKE_EXTRA_TARGETS += codesign dmg
} else {
    BINDIST_SOURCE = "$(INSTALL_ROOT)$$QTC_PREFIX"
    deployqt.commands = $$PWD/scripts/deployqt.py -i \"$(INSTALL_ROOT)$$QTC_PREFIX\"
    deployqt.depends = install
    win32 {
        deployartifacts.depends = install
        deployartifacts.commands = git clone "git://gitorious.org/qt-creator/binary-artifacts.git"&& xcopy /s /q /y /i "binary-artifacts\\win32" \"$(INSTALL_ROOT)$$QTC_PREFIX\"&& rmdir /s /q binary-artifacts
        QMAKE_EXTRA_TARGETS += deployartifacts
    }
}

bindist.commands = 7z a -mx9 $$OUT_PWD/qt-creator-$${PATTERN}.7z \"$$BINDIST_SOURCE\"

win32 {
    deployqt.commands ~= s,/,\\\\,g
    bindist.commands ~= s,/,\\\\,g
}

bindist.depends = deployqt

QMAKE_EXTRA_TARGETS += deployqt bindist
