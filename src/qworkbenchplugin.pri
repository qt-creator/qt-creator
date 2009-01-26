isEmpty(IDE_BUILD_TREE) {
  IDE_BUILD_TREE = $$OUT_PWD/../../../
}
include(qworkbench.pri)

isEmpty(PROVIDER) {
    PROVIDER = Nokia
}

DESTDIR = $$IDE_PLUGIN_PATH/$$PROVIDER/
LIBS += -L$$DESTDIR
INCLUDEPATH += $$IDE_SOURCE_TREE/src/plugins
DEPENDPATH += $$IDE_SOURCE_TREE/src/plugins

# copy the plugin spec
isEmpty(TARGET) {
    error("qworkbenchplugin.pri: You must provide a TARGET")
}

# Copy the pluginspec file to the liberary directyory.
# Note: On Windows/MinGW with some sh.exe in the path,
# QMAKE_COPY is some cp command that does not understand
# "\". Force the standard windows copy.
COPYDEST = $${DESTDIR}
COPYSRC = $${_PRO_FILE_PWD_}/$${TARGET}.pluginspec

TARGET = $$qtLibraryTarget($$TARGET)

win32 {
    COPYDEST ~= s|/+|\|
    COPYSRC ~= s|/+|\|
    COPY_CMD=xcopy /y
} else {
    COPY_CMD=$${QMAKE_COPY}
}

QMAKE_POST_LINK += $${COPY_CMD} $${COPYSRC} $${COPYDEST}

macx {
        QMAKE_LFLAGS_SONAME = -Wl,-install_name,@executable_path/../PlugIns/$${PROVIDER}/
} else:linux-* {
    #do the rpath by hand since it's not possible to use ORIGIN in QMAKE_RPATHDIR
    QMAKE_RPATHDIR += \$\$ORIGIN
    QMAKE_RPATHDIR += \$\$ORIGIN/..
    QMAKE_RPATHDIR += \$\$ORIGIN/../..
    IDE_PLUGIN_RPATH = $$join(QMAKE_RPATHDIR, ":")
    QMAKE_LFLAGS += -Wl,-z,origin \'-Wl,-rpath,$${IDE_PLUGIN_RPATH}\'
    QMAKE_RPATHDIR =
}


contains(QT_CONFIG, reduce_exports):CONFIG += hide_symbols

CONFIG += plugin plugin_with_soname

linux-* {
    target.path = /lib/qtcreator/plugins/$$PROVIDER
    pluginspec.files += $${TARGET}.pluginspec
    pluginspec.path = /lib/qtcreator/plugins/$$PROVIDER
    INSTALLS += target pluginspec
}
