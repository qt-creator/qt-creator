TEMPLATE = subdirs
SUBDIRS = qtcreator/static.pro

include(../qtcreator.pri)

linux {
    appdata = $$cat($$PWD/metainfo/org.qt-project.qtcreator.appdata.xml.cmakein, blob)
    appdata = $$replace(appdata, \\$\\{IDE_VERSION_DISPLAY\\}, $$QTCREATOR_DISPLAY_VERSION)
    appdata = $$replace(appdata, \\$\\{DATE_ATTRIBUTE\\}, "")
    write_file($$OUT_PWD/metainfo/org.qt-project.qtcreator.appdata.xml, appdata)

    appstream.files = $$OUT_PWD/metainfo/org.qt-project.qtcreator.appdata.xml
    appstream.path = $$QTC_PREFIX/share/metainfo/

    desktop.files = $$PWD/applications/org.qt-project.qtcreator.desktop
    desktop.path = $$QTC_PREFIX/share/applications/

    INSTALLS += appstream desktop
}

defineTest(hasLupdate) {
    cmd = $$eval(QT_TOOL.lupdate.binary)
    isEmpty(cmd) {
        cmd = $$[QT_HOST_BINS]/lupdate
        contains(QMAKE_HOST.os, Windows):exists($${cmd}.exe): return(true)
        contains(QMAKE_HOST.os, Darwin):exists($${cmd}.app/Contents/MacOS/lupdate): return(true)
        exists($$cmd): return(true)
    } else {
        exists($$last(cmd)): return(true)
    }
    return(false)
}

hasLupdate(): SUBDIRS += qtcreator/translations

DISTFILES += share.qbs \
    ../src/share/share.qbs
