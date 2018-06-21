include(../qttest.pri)

TMP_ILP = $$IDE_LIBEXEC_PATH
win32: TMP_ILP = $$replace(TMP_ILP, \\\\, /)

DEFINES += $$shell_quote(SDKTOOL_DIR=\"$$TMP_ILP\")

SOURCES += tst_sdktool.cpp
