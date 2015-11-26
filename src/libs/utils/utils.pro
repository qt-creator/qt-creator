include(../../qtcreatorlibrary.pri)
include(utils-lib.pri)

DEFINES += QTC_REL_TOOLS_PATH=$$shell_quote(\"$$relative_path($$IDE_LIBEXEC_PATH, $$IDE_BIN_PATH)\")
