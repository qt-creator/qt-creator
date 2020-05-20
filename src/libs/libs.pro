include(../../qtcreator.pri)

TEMPLATE  = subdirs

SUBDIRS   += \
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
    clangsupport \
    languageserverprotocol \
    sqlite

qtHaveModule(quick) {
    SUBDIRS += \
        tracing
}

QTC_DO_NOT_BUILD_QMLDESIGNER = $$(QTC_DO_NOT_BUILD_QMLDESIGNER)
isEmpty(QTC_DO_NOT_BUILD_QMLDESIGNER):qtHaveModule(quick-private) {
    exists($$[QT_INSTALL_QML]/QtQuick/Controls/qmldir) {
        SUBDIRS += advanceddockingsystem
    }
}

for(l, SUBDIRS) {
    QTC_LIB_DEPENDS =
    include($$l/$${l}_dependencies.pri)
    lv = $${l}.depends
    $$lv = $$QTC_LIB_DEPENDS
}

SUBDIRS += \
    utils/process_stub.pro

include(../shared/yaml-cpp/yaml-cpp_installation.pri)
isEmpty(EXTERNAL_YAML_CPP_FOUND): SUBDIRS += 3rdparty/yaml-cpp

isEmpty(KSYNTAXHIGHLIGHTING_LIB_DIR): KSYNTAXHIGHLIGHTING_LIB_DIR=$$(KSYNTAXHIGHLIGHTING_LIB_DIR)
!isEmpty(KSYNTAXHIGHLIGHTING_LIB_DIR) {
    # enable short information message
    KSYNTAX_WARN_ON = 1
}

include(../shared/syntax/syntax_shared.pri)
isEmpty(KSYNTAXHIGHLIGHTING_LIB_DIR) {
    SUBDIRS += \
        3rdparty/syntax-highlighting \
        3rdparty/syntax-highlighting/data

    equals(KSYNTAX_WARN_ON, 1) {
        message("Either KSYNTAXHIGHLIGHTING_LIB_DIR does not exist or include path could not be deduced.")
        unset(KSYNTAX_WARN_ON)
    }
} else {
    message("Using KSyntaxHighlighting provided at $${KSYNTAXHIGHLIGHTING_LIB_DIR}.")
}

win32:SUBDIRS += utils/process_ctrlc_stub.pro

# Windows: Compile Qt Creator CDB extension if Debugging tools can be detected.
win32: isEmpty(QTC_SKIP_CDBEXT) {
    include(qtcreatorcdbext/cdb_detect.pri)
    !isEmpty(CDB_PATH): exists($$CDB_PATH) {
        SUBDIRS += qtcreatorcdbext
    } else {
        message("Compiling Qt Creator without a CDB extension.")
        message("If CDB is installed in a none default path define a CDB_PATH")
        message("environment variable pointing to your CDB installation.")
    }
}

QMAKE_EXTRA_TARGETS += deployqt # dummy
