include(../../qtcreator.pri)

isEmpty(TEMPLATE):TEMPLATE=app
CONFIG += qt warn_on console depend_includepath testcase qtestlib
CONFIG -= app_bundle

symbian:{
    TARGET.EPOCHEAPSIZE = 0x100000 0x2000000
#    DEFINES += QTEST_NO_SPECIALIZATIONS
    TARGET.CAPABILITY="None"
    RSS_RULES ="group_name=\"QtTests\";"
}

linux-* {
    QMAKE_RPATHDIR += $$IDE_BUILD_TREE/$$IDE_LIBRARY_BASENAME/qtcreator
    QMAKE_RPATHDIR += $$IDE_PLUGIN_PATH/Nokia

    IDE_PLUGIN_RPATH = $$join(QMAKE_RPATHDIR, ":")

    QMAKE_LFLAGS += -Wl,-z,origin \'-Wl,-rpath,$${IDE_PLUGIN_RPATH}\'
} else:macx {
    QMAKE_LFLAGS += -Wl,-rpath,\"$$IDE_BIN_PATH/../\"
}

# prefix test binary with tst_
!contains(TARGET, ^tst_.*):TARGET = $$join(TARGET,,"tst_")
