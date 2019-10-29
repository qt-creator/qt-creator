# set RPATH_BASE to the IDE_..._PATH of the target

isEmpty(RPATH_BASE): \
    error("You must set RPATH_BASE before including rpath.pri")

REL_PATH_TO_LIBS = $$relative_path($$IDE_LIBRARY_PATH, $$RPATH_BASE)
REL_PATH_TO_PLUGINS = $$relative_path($$IDE_PLUGIN_PATH, $$RPATH_BASE)

macos {
    QMAKE_LFLAGS += -Wl,-rpath,@loader_path/$$REL_PATH_TO_LIBS,-rpath,@loader_path/$$REL_PATH_TO_PLUGINS
} else:linux-* {
    QMAKE_RPATHDIR += \$\$ORIGIN
    QMAKE_RPATHDIR += \$\$ORIGIN/$$REL_PATH_TO_LIBS
    QMAKE_RPATHDIR += \$\$ORIGIN/$$REL_PATH_TO_PLUGINS

    IDE_PLUGIN_RPATH = $$join(QMAKE_RPATHDIR, ":")
    QMAKE_LFLAGS += -Wl,-z,origin \'-Wl,-rpath,$${IDE_PLUGIN_RPATH}\'
    QMAKE_RPATHDIR =
}
