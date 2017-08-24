include(../../qtcreator.pri)

TEMPLATE  = subdirs

SUBDIRS   = \
    aggregation \
    extensionsystem \
    utils \
    languageutils \
    cplusplus \
    modelinglib \
    qmljs \
    qmldebug \
    qmleditorwidgets \
    glsl \
    ssh \
    sqlite \
    clangsupport

qtHaveModule(quick) {
    SUBDIRS += \
        flamegraph \
        timeline
}

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
win32: isEmpty(QTC_SKIP_CDBEXT) {
    include(qtcreatorcdbext/cdb_detect.pri)
    exists($$CDB_PATH) {
        SUBDIRS += qtcreatorcdbext
    } else {
        message("Compiling Qt Creator without a CDB extension.")
        message("If CDB is installed in a none default path define a CDB_PATH")
        message("environment variable pointing to your CDB installation.")
    }
}
