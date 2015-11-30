include(../qttest.pri)

DEFINES += "SDKTOOL_DIR=\\\"$$replace(IDE_LIBEXEC_PATH, " ", "\\ ")\\\""

SOURCES += tst_sdktool.cpp
