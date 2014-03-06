include(../../qtcreator.pri)

TEMPLATE  = subdirs

SUBDIRS   = \
    aggregation \
    extensionsystem \
    utils \
    languageutils \
    cplusplus \
    qmljs \
    qmldebug \
    qmleditorwidgets \
    glsl \
    ssh \
    zeroconf

for(l, SUBDIRS) {
    QTC_LIB_DEPENDS =
    include($$l/$${l}_dependencies.pri)
    lv = $${l}.depends
    $$lv = $$QTC_LIB_DEPENDS
}

SUBDIRS += \
    utils/process_stub.pro

minQtVersion(5, 0, 0) {
    QBS_DIRS = \
        corelib \
        qtprofilesetup \
        ../shared/qbs/src/plugins \
        ../shared/qbs/static.pro
    corelib.subdir = ../shared/qbs/src/lib/corelib
    qtprofilesetup.subdir = ../shared/qbs/src/lib/qtprofilesetup
    qtprofilesetup.depends = corelib

    exists(../shared/qbs/qbs.pro): SUBDIRS += $$QBS_DIRS
    TR_EXCLUDE = $$QBS_DIRS
}

win32:SUBDIRS += utils/process_ctrlc_stub.pro

# Windows: Compile Qt Creator CDB extension if Debugging tools can be detected.    
win32 {
    include(qtcreatorcdbext/cdb_detect.pri)
    exists($$CDB_PATH):SUBDIRS += qtcreatorcdbext
}
