macx {
    # this is needed for the binary itself?
    QMAKE_LFLAGS_SONAME = -Wl,-install_name,@executable_path/../PlugIns/
} else:linux-* {
    #do the rpath by hand since it's not possible to use ORIGIN in QMAKE_RPATHDIR
    QMAKE_RPATHDIR += \$\$ORIGIN/../lib/qtcreator
    IDE_PLUGIN_RPATH = $$join(QMAKE_RPATHDIR, ":")
    message($$QMAKE_RPATHDIR)
    message($$IDE_PLUGIN_RPATH)
 
    QMAKE_LFLAGS += -Wl,-z,origin \'-Wl,-rpath,$${IDE_PLUGIN_RPATH}\'
    QMAKE_RPATHDIR =
}


