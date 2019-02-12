TEMPLATE = subdirs

QBS_DIRS = \
    qbscorelib \
    qbsapps \
    qbslibexec \
    qbsplugins \
    qbsstatic

qbscorelib.subdir = qbs/src/lib/corelib
qbsapps.subdir = qbs/src/app
qbsapps.depends = qbscorelib
qbslibexec.subdir = qbs/src/libexec
qbslibexec.depends = qbscorelib
qbsplugins.subdir = qbs/src/plugins
qbsplugins.depends = qbscorelib
qbsstatic.file = qbs/static.pro

exists(qbs/qbs.pro) {
    isEmpty(QBS_INSTALL_DIR):QBS_INSTALL_DIR = $$(QBS_INSTALL_DIR)
    isEmpty(QBS_INSTALL_DIR):SUBDIRS += $$QBS_DIRS
}
TR_EXCLUDE = qbs

QMAKE_EXTRA_TARGETS += deployqt # dummy
