QTC_PLUGIN_NAME = TextEditor
QTC_LIB_DEPENDS += \
    aggregation \
    extensionsystem \
    utils

QTC_PLUGIN_DEPENDS += \
    coreplugin

# needed for plugins that depend on TextEditor plugin
include(../../shared/syntax/syntax_shared.pri)
!isEmpty(KSYNTAXHIGHLIGHTING_LIB_DIR):!isEmpty(KSYNTAXHIGHLIGHTING_INCLUDE_DIR) {
    INCLUDEPATH *= $${KSYNTAXHIGHLIGHTING_INCLUDE_DIR}
    LIBS *= -L$$KSYNTAXHIGHLIGHTING_LIB_DIR -lKF5SyntaxHighlighting

    linux {
        QMAKE_LFLAGS += -Wl,-z,origin
        QMAKE_LFLAGS += -Wl,-rpath,$$shell_quote($${KSYNTAXHIGHLIGHTING_LIB_DIR})
    }
} else {
    QTC_LIB_DEPENDS += syntax-highlighting
}
