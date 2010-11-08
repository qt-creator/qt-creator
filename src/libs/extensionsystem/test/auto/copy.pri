#set COPYFILES and COPYDIR

# stolen from qtcreatorplugin.pri
copy2build.input = COPYFILES
copy2build.output = $$COPYDIR/${QMAKE_FUNC_FILE_IN_stripSrcDir}
isEmpty(vcproj):copy2build.variable_out = PRE_TARGETDEPS
copy2build.commands = $$QMAKE_COPY ${QMAKE_FILE_IN} ${QMAKE_FILE_OUT}
copy2build.name = COPY ${QMAKE_FILE_IN}
copy2build.CONFIG += no_link
QMAKE_EXTRA_COMPILERS += copy2build

