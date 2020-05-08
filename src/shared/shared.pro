TEMPLATE = subdirs

QBS_DIRS = \
    qbscorelib \
    qbsapps \
    qbslibexec \
    qbsmsbuildlib \
    qbsplugins \
    qbsstatic

qbscorelib.subdir = qbs/src/lib/corelib
qbsapps.subdir = qbs/src/app
qbsapps.depends = qbscorelib
qbslibexec.subdir = qbs/src/libexec
qbslibexec.depends = qbscorelib
qbsmsbuildlib.subdir = qbs/src/lib/msbuild
qbsmsbuildlib.depends = qbscorelib
qbsplugins.subdir = qbs/src/plugins
qbsplugins.depends = qbscorelib qbsmsbuildlib
qbsstatic.file = qbs/static.pro

exists(qbs/qbs.pro) {
    isEmpty(QBS_INSTALL_DIR):QBS_INSTALL_DIR = $$(QBS_INSTALL_DIR)
    isEmpty(QBS_INSTALL_DIR):SUBDIRS += $$QBS_DIRS

    include(qbs/src/lib/bundledlibs.pri)
    qbs_use_bundled_qtscript {
        qbsscriptenginelib.file = qbs/src/lib/scriptengine/scriptengine.pro
        qbscorelib.depends = qbsscriptenginelib
        SUBDIRS += qbsscriptenginelib
    }
}
TR_EXCLUDE = qbs

QMAKE_EXTRA_TARGETS += deployqt # dummy
