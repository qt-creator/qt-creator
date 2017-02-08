TEMPLATE = subdirs
SUBDIRS = qtcreator/static.pro

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
