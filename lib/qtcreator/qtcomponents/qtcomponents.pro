include(../../../qtcreator.pri)

TEMPLATE = app
TARGET = phony_target3
CONFIG -= qt separate_debug_info gdb_dwarf_index
QT =
LIBS =
macx:CONFIG -= app_bundle

isEmpty(vcproj) {
    QMAKE_LINK = @: IGNORE THIS LINE
    OBJECTS_DIR =
    win32:CONFIG -= embed_manifest_exe
} else {
    CONFIG += console
    PHONY_DEPS = .
    phony_src.input = PHONY_DEPS
    phony_src.output = phony.c
    phony_src.variable_out = GENERATED_SOURCES
    phony_src.commands = echo int main() { return 0; } > phony.c
    phony_src.name = CREATE phony.c
    phony_src.CONFIG += combine
    QMAKE_EXTRA_COMPILERS += phony_src
}

DATA_FILE_PATTERNS = \
    $$PWD/*.qml \
    $$PWD/qmldir \
    $$PWD/images/* \
    $$PWD/custom/* \
    $$PWD/custom/behaviors/* \
    $$PWD/custom/private/*

!isEmpty(copydata) {

    for(data_file, DATA_FILE_PATTERNS) {
        files = $$files($$data_file, false)
        win32:files ~= s|\\\\|/|g
        for(file, files):!exists($$file/*):FILES += $$file
    }

    OTHER_FILES += $$FILES
    copy2build.input = FILES
    copy2build.output = $$IDE_LIBRARY_PATH/qtcomponents/${QMAKE_FUNC_FILE_IN_stripSrcDir}
    isEmpty(vcproj):copy2build.variable_out = PRE_TARGETDEPS
    win32:copy2build.commands = $$QMAKE_COPY \"${QMAKE_FILE_IN}\" \"${QMAKE_FILE_OUT}\"
    unix:copy2build.commands = $$QMAKE_COPY ${QMAKE_FILE_IN} ${QMAKE_FILE_OUT}
    copy2build.name = COPY ${QMAKE_FILE_IN}
    copy2build.CONFIG += no_link
    QMAKE_EXTRA_COMPILERS += copy2build
}

!macx {
    qmlfiles.files = $$PWD/*.qml $$PWD/qmldir $$PWD/images $$PWD/custom
    qmlfiles.path = $$QTC_PREFIX/$${IDE_LIBRARY_BASENAME}/qtcreator/qtcomponents
    INSTALLS += qmlfiles
}
