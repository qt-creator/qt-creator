TEMPLATE = lib
TARGET = simple_test_plugin
CONFIG += shared

SOURCES += simple_test_plugin.cpp

macx {
   QMAKE_LFLAGS_SONAME = -Wl,-install_name,@executable_path/../PlugIns/
} else:linux-* {
    #do the rpath by hand since it's not possible to use ORIGIN in QMAKE_RPATHDIR
    QMAKE_RPATHDIR += \$\$ORIGIN/..
    IDE_PLUGIN_RPATH = $$join(QMAKE_RPATHDIR, ":")
    QMAKE_LFLAGS += -Wl,-z,origin \'-Wl,-rpath,$${IDE_PLUGIN_RPATH}\'
    QMAKE_RPATHDIR =
}

maemo5 {
    target.path = /opt/usr/lib
    INSTALLS += target
}
