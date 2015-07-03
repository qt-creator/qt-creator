include(../qtcreator.pri)
include(rpath.pri)

TEMPLATE = app

CONFIG += console
CONFIG -= app_bundle

DESTDIR = $$IDE_LIBEXEC_PATH
target.path = $$QTC_PREFIX/$$relative_path($$IDE_LIBEXEC_PATH, $$IDE_BUILD_TREE)
INSTALLS += target
