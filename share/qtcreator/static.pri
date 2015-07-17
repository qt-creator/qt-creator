# This pri file is used to deploy files that are not compiled while building
# Qt Creator. It handles copying of files into the build directory if using
# a shadow build and adds the respective install target as well.
#
# Usage: Define variables (details below) and include this pri file afterwards.
#
# STATIC_BASE    - base directory for the files listed in STATIC_FILES
# STATIC_FILES   - list of files to be deployed

include(../../qtcreator.pri)

# used in custom compilers which just copy files
defineReplace(stripStaticBase) {
    return($$relative_path($$1, $$STATIC_BASE))
}

# handle conditional copying; copydata will be set by qtcreator.pri
!isEmpty(STATIC_FILES) {
    isEmpty(STATIC_BASE): \
        error("Using STATIC_FILES without having STATIC_BASE set")

    !isEmpty(copydata) {
        copy2build.input += STATIC_FILES
        copy2build.output = $$IDE_DATA_PATH/${QMAKE_FUNC_FILE_IN_stripStaticBase}
        isEmpty(vcproj):copy2build.variable_out = PRE_TARGETDEPS
        win32:copy2build.commands = $$QMAKE_COPY \"${QMAKE_FILE_IN}\" \"${QMAKE_FILE_OUT}\"
        unix:copy2build.commands = $$QMAKE_COPY ${QMAKE_FILE_IN} ${QMAKE_FILE_OUT}
        copy2build.name = COPY ${QMAKE_FILE_IN}
        copy2build.config += no_link
        QMAKE_EXTRA_COMPILERS += copy2build
    }

    !osx {
        static.files = $$STATIC_FILES
        static.base = $$STATIC_BASE
        static.path = $$QTC_PREFIX/share/qtcreator
        INSTALLS += static
    }
}
