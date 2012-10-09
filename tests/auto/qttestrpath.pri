linux-* {
    QMAKE_RPATHDIR += $$IDE_BUILD_TREE/$$IDE_LIBRARY_BASENAME/qtcreator
    QMAKE_RPATHDIR += $$IDE_PLUGIN_PATH/QtProject

    IDE_PLUGIN_RPATH = $$join(QMAKE_RPATHDIR, ":")

    QMAKE_LFLAGS += -Wl,-z,origin \'-Wl,-rpath,$${IDE_PLUGIN_RPATH}\'
} else:macx {
    QMAKE_LFLAGS += -Wl,-rpath,\"$$IDE_BIN_PATH/../\"
}

