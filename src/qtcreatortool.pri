include(../qtcreator.pri)

TEMPLATE = app

CONFIG += console
CONFIG -= app_bundle

DESTDIR = $$IDE_LIBEXEC_PATH
REL_PATH_TO_LIBS = $$relative_path($$IDE_LIBRARY_PATH, $$IDE_LIBEXEC_PATH)
REL_PATH_TO_PLUGINS = $$relative_path($$IDE_PLUGIN_PATH, $$IDE_LIBEXEC_PATH)
osx {
    QMAKE_LFLAGS += -Wl,-rpath,@executable_path/$$REL_PATH_TO_LIBS,-rpath,@executable_path/$$REL_PATH_TO_PLUGINS
} else {
    QMAKE_RPATHDIR += \$\$ORIGIN/$$REL_PATH_TO_LIBS
    QMAKE_RPATHDIR += \$\$ORIGIN/$$REL_PATH_TO_PLUGINS
}
include(rpath.pri)

target.path = $$QTC_PREFIX/$$relative_path($$IDE_LIBEXEC_PATH, $$IDE_BUILD_TREE)
INSTALLS += target
