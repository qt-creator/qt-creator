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
    deployqt.commands = macdeployqt "$${APPBUNDLE}" \
        -executable="$${APPBUNDLE}/Contents/MacOS/qmlpuppet.app/Contents/MacOS/qmlpuppet" \
        -executable="$${APPBUNDLE}/Contents/Resources/qtpromaker" \
        -executable="$${APPBUNDLE}/Contents/MacOS/qmlprofiler"
    deployqt.depends = default
    bindist.commands = 7z a -mx9 $$OUT_PWD/qtcreator$(INSTALL_POSTFIX).7z "$$OUT_PWD/bin/Qt Creator.app/"
} else {
    deployqt.commands = $$PWD/scripts/deployqt.py -i $(INSTALL_ROOT)
    win32:deployqt.commands ~= s,/,\\\\,g
    deployqt.depends = install
    bindist.commands = 7z a -mx9 $$OUT_PWD/qtcreator$(INSTALL_POSTFIX).7z $(INSTALL_ROOT)
    win32:bindist.commands ~= s,/,\\\\,g
}
bindist.depends = deployqt
QMAKE_EXTRA_TARGETS += deployqt bindist
