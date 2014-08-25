TARGET = qtcrashhandler

TEMPLATE = app
QT_BREAKPAD_ROOT_PATH = $$(QT_BREAKPAD_ROOT_PATH)
include($${QT_BREAKPAD_ROOT_PATH}/qtcrashhandler.pri)

include(../../../qtcreator.pri)
DESTDIR = $$IDE_BIN_PATH
include(../../rpath.pri)
