include(../../qtcreator.pri)

win32:i_flag = i
defineReplace(stripSrcDir) {
    win32 {
        !contains(1, ^.:.*):1 = $$OUT_PWD/$$1
    } else {
        !contains(1, ^/.*):1 = $$OUT_PWD/$$1
    }
    out = $$cleanPath($$1)
    out ~= s|^$$re_escape($$PWD/)||$$i_flag
    return($$out)
}

contains(TEMPLATE, vc.*)|contains(TEMPLATE_PREFIX, vc):vcproj = 1

TEMPLATE = app
TARGET = phony_target

isEmpty(vcproj) {
    QMAKE_LINK = : IGNORE REST
    OBJECTS_DIR =
    win32:CONFIG -= embed_manifest_exe
} else {
    PHONY_DEPS = .
    phony_src.input = PHONY_DEPS
    phony_src.output = phony.c
    phony_src.commands = echo int main() { return 0; } > phony.c
    phony_src.CONFIG += combine
    QMAKE_EXTRA_COMPILERS += phony_src
}

DATA_DIRS = \
    snippets \
    templates \
    designer \
    schemes \
    gdbmacros

macx|!equals(_PRO_FILE_PWD_, $$OUT_PWD) {

    for(data_dir, DATA_DIRS) {
        files = $$files($$PWD/$$data_dir/*.*, true)
        win32:files ~= s|\\\\|/|g
        FILES += $$files
    }

    copy2build.input = FILES
    copy2build.output = $$IDE_DATA_PATH/${QMAKE_FUNC_FILE_IN_stripSrcDir}
    isEmpty(vcproj):copy2build.variable_out = PRE_TARGETDEPS
    copy2build.commands = $$QMAKE_COPY \"${QMAKE_FILE_IN}\" \"${QMAKE_FILE_OUT}\"
    copy2build.name = COPY ${QMAKE_FILE_IN}
    copy2build.CONFIG += no_link
    QMAKE_EXTRA_COMPILERS += copy2build

    macx {
        run_in_term.target = $$IDE_DATA_PATH/runInTerminal.command
        run_in_term.depends = $$PWD/runInTerminal.command
        run_in_term.commands = $$QMAKE_COPY $< $@
        QMAKE_EXTRA_TARGETS += run_in_term
        PRE_TARGETDEPS += $$run_in_term.target
        QMAKE_CLEAN += $$run_in_term.target
    }
}

unix:!macx {
    for(data_dir, DATA_DIRS) {
        eval($${data_dir}.files = $$quote($$PWD/$$data_dir))
        eval($${data_dir}.path = /share/qtcreator)
        INSTALLS += $$data_dir
    }
}
