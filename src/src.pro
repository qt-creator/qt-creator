include(../qtcreator.pri)

TEMPLATE  = subdirs
CONFIG   += ordered

QBS_DIRS = \
    qbscorelib \
    qbsqtprofilesetup \
    qbsapps \
    qbslibexec \
    qbsplugins \
    qbsstatic

qbscorelib.subdir = shared/qbs/src/lib/corelib
qbsqtprofilesetup.subdir = shared/qbs/src/lib/qtprofilesetup
qbsqtprofilesetup.depends = qbscorelib
qbsapps.subdir = shared/qbs/src/app
qbsapps.depends = qbsqtprofilesetup
qbslibexec.subdir = shared/qbs/src/libexec
qbslibexec.depends = qbscorelib
qbsplugins.subdir = shared/qbs/src/plugins
qbsstatic.file = shared/qbs/static.pro

exists(shared/qbs/qbs.pro) {
    isEmpty(QBS_INSTALL_DIR):QBS_INSTALL_DIR = $$(QBS_INSTALL_DIR)
    isEmpty(QBS_INSTALL_DIR):SUBDIRS += $$QBS_DIRS
}
TR_EXCLUDE = shared/qbs

SUBDIRS += \
    libs \
    app \
    plugins \
    tools \
    share/qtcreator/data.pro \
    share/3rdparty/data.pro
