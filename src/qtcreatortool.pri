include(../qtcreator.pri)
include(rpath.pri)

TEMPLATE = app

CONFIG += console
CONFIG -= app_bundle

DESTDIR = $${IDE_LIBEXEC_PATH}
target.path  = $${QTC_PREFIX}/bin # FIXME: libexec, more or less
INSTALLS += target
