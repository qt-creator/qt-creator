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
    timeline \
    sqlite \
    codemodelbackendipc

for(l, SUBDIRS) {
    QTC_LIB_DEPENDS =
    include($$l/$${l}_dependencies.pri)
    lv = $${l}.depends
    $$lv = $$QTC_LIB_DEPENDS
}

SUBDIRS += \
    utils/process_stub.pro

win32:SUBDIRS += utils/process_ctrlc_stub.pro

# Windows: Compile Qt Creator CDB extension if Debugging tools can be detected.
win32 {
    include(qtcreatorcdbext/cdb_detect.pri)
    exists($$CDB_PATH):SUBDIRS += qtcreatorcdbext
}
