include(../qtcreator.pri)

TEMPLATE = app

CONFIG += console
CONFIG -= app_bundle

DESTDIR = $$IDE_LIBEXEC_PATH

RPATH_BASE = $$IDE_LIBEXEC_PATH
include(rpath.pri)

target.path  = $$INSTALL_LIBEXEC_PATH
INSTALLS += target
