TEMPLATE = subdirs

QBS_DIRS = \
    qbscorelib \
    qbsqtprofilesetup \
    qbsapps \
    qbslibexec \
    qbsplugins \
    qbsstatic

qbscorelib.subdir = qbs/src/lib/corelib
qbsqtprofilesetup.subdir = qbs/src/lib/qtprofilesetup
qbsqtprofilesetup.depends = qbscorelib
qbsapps.subdir = qbs/src/app
qbsapps.depends = qbsqtprofilesetup
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
