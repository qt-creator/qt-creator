QT        -= gui

include(../../../qtcreator.pri)

TEMPLATE  = app
TARGET    = qtpromaker
DESTDIR   = $$IDE_LIBEXEC_PATH

CONFIG    += console warn_on
CONFIG    -= app_bundle

SOURCES   += main.cpp

target.path  = $$QTC_PREFIX/bin # FIXME: libexec, more or less
INSTALLS    += target
