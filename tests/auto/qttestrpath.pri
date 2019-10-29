linux-* {
    QMAKE_RPATHDIR += $$IDE_LIBRARY_PATH
    QMAKE_RPATHDIR += $$IDE_PLUGIN_PATH

    IDE_PLUGIN_RPATH = $$join(QMAKE_RPATHDIR, ":")

    QMAKE_LFLAGS += -Wl,-z,origin \'-Wl,-rpath,$${IDE_PLUGIN_RPATH}\'
} else:macx {
    QMAKE_LFLAGS += -Wl,-rpath,\"$$IDE_LIBRARY_PATH\",-rpath,\"$$IDE_PLUGIN_PATH\"
}

