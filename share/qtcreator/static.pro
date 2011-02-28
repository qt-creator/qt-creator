include(../../qtcreator.pri)

TEMPLATE = app
TARGET = phony_target
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

DATA_DIRS = \
    examplebrowser \
    snippets \
    templates \
    designer \
    schemes \
    styles \
    rss \
    gdbmacros \
    qmldesigner \
    qmlicons \
    qml \
    qml-type-descriptions \
    generic-highlighter \
    glsl

# files that are to be unconditionally "deployed" to the build dir from src/share to share
defineReplace(stripSrcResourceDir) {
    win32 {
        !contains(1, ^.:.*):1 = $$OUT_PWD/$$1
    } else {
        !contains(1, ^/.*):1 = $$OUT_PWD/$$1
    }
    out = $$cleanPath($$1)
    out ~= s|^$$re_escape($$IDE_SOURCE_TREE/src/share/qtcreator/)||$$i_flag
    return($$out)
}
DATA_FILES_SRC = \
    externaltools/lrelease.xml \
    externaltools/lupdate.xml \
    externaltools/sort.xml
linux-*:DATA_FILES_SRC += externaltools/vi.xml
macx:DATA_FILES_SRC += externaltools/vi_mac.xml
win32:DATA_FILES_SRC += externaltools/notepad_win.xml
win32:DATA_FILES_SRC ~= s|\\\\|/|g
for(file, DATA_FILES_SRC):DATA_FILES += $$IDE_SOURCE_TREE/src/share/qtcreator/$$file
macx:OTHER_FILES += $$DATA_FILES
unconditionalCopy2build.input = DATA_FILES
unconditionalCopy2build.output = $$IDE_DATA_PATH/${QMAKE_FUNC_FILE_IN_stripSrcResourceDir}
isEmpty(vcproj):unconditionalCopy2build.variable_out = PRE_TARGETDEPS
win32:unconditionalCopy2build.commands = $$QMAKE_COPY \"${QMAKE_FILE_IN}\" \"${QMAKE_FILE_OUT}\"
unix:unconditionalCopy2build.commands = $$QMAKE_COPY ${QMAKE_FILE_IN} ${QMAKE_FILE_OUT}
unconditionalCopy2build.name = COPY ${QMAKE_FILE_IN}
unconditionalCopy2build.CONFIG += no_link
QMAKE_EXTRA_COMPILERS += unconditionalCopy2build

# conditionally deployed data
!isEmpty(copydata) {

    for(data_dir, DATA_DIRS) {
        files = $$files($$PWD/$$data_dir/*, true)
        win32:files ~= s|\\\\|/|g
        for(file, files):!exists($$file/*):FILES += $$file
    }

    macx:OTHER_FILES += $$FILES
    copy2build.input = FILES
    copy2build.output = $$IDE_DATA_PATH/${QMAKE_FUNC_FILE_IN_stripSrcDir}
    isEmpty(vcproj):copy2build.variable_out = PRE_TARGETDEPS
    win32:copy2build.commands = $$QMAKE_COPY \"${QMAKE_FILE_IN}\" \"${QMAKE_FILE_OUT}\"
    unix:copy2build.commands = $$QMAKE_COPY ${QMAKE_FILE_IN} ${QMAKE_FILE_OUT}
    copy2build.name = COPY ${QMAKE_FILE_IN}
    copy2build.CONFIG += no_link
    QMAKE_EXTRA_COMPILERS += copy2build
}

!macx {
    for(data_dir, DATA_DIRS) {
        eval($${data_dir}.files = $$quote($$PWD/$$data_dir))
        eval($${data_dir}.path = /share/qtcreator)
        INSTALLS += $$data_dir
    }
}
