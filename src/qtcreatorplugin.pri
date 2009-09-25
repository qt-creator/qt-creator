include(../qtcreator.pri)

isEmpty(PROVIDER) {
    PROVIDER = Nokia
}

DESTDIR = $$IDE_PLUGIN_PATH/$$PROVIDER
LIBS += -L$$DESTDIR
INCLUDEPATH += $$IDE_SOURCE_TREE/src/plugins
DEPENDPATH += $$IDE_SOURCE_TREE/src/plugins

# copy the plugin spec
isEmpty(TARGET) {
    error("qtcreatorplugin.pri: You must provide a TARGET")
}

PLUGINSPECS = $${_PRO_FILE_PWD_}/$${TARGET}.pluginspec
copy2build.input = PLUGINSPECS
copy2build.output = $$DESTDIR/${QMAKE_FUNC_FILE_IN_stripSrcDir}
isEmpty(vcproj):copy2build.variable_out = PRE_TARGETDEPS
copy2build.commands = $$QMAKE_COPY ${QMAKE_FILE_IN} ${QMAKE_FILE_OUT}
copy2build.name = COPY ${QMAKE_FILE_IN}
copy2build.CONFIG += no_link
QMAKE_EXTRA_COMPILERS += copy2build

TARGET = $$qtLibraryTarget($$TARGET)

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

!macx {
    target.path = /$$IDE_LIBRARY_BASENAME/qtcreator/plugins/$$PROVIDER
    pluginspec.files += $${TARGET}.pluginspec
    pluginspec.path = /$$IDE_LIBRARY_BASENAME/qtcreator/plugins/$$PROVIDER
    INSTALLS += target pluginspec
}
