TEMPLATE = app
TARGET = phony_target
CONFIG -= qt sdk separate_debug_info gdb_dwarf_index
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

STATIC_BASE = $$PWD

# files/folders that are conditionally "deployed" to the build directory
DATA_DIRS = \
    welcomescreen \
    examplebrowser \
    snippets \
    templates \
    themes \
    designer \
    schemes \
    styles \
    rss \
    debugger \
    qmldesigner \
    qmlicons \
    qml \
    qml-type-descriptions \
    generic-highlighter \
    glsl \
    cplusplus
macx: DATA_DIRS += scripts

for(data_dir, DATA_DIRS) {
    files = $$files($$PWD/$$data_dir/*, true)
    # Info.plist.in are handled below
    for(file, files):!contains(file, ".*/Info\\.plist\\.in$"):!exists($$file/*): \
        STATIC_FILES += $$file
}

include(static.pri)

# stuff that cannot be handled by static.pri
osx {
   # do version magic for app bundles
   dumpinfo.input = qml/qmldump/Info.plist.in
   dumpinfo.output = $$IDE_DATA_PATH/qml/qmldump/Info.plist
   QMAKE_SUBSTITUTES += dumpinfo
}

SRCRESOURCEDIR = $$IDE_SOURCE_TREE/src/share/qtcreator/
defineReplace(stripSrcResourceDir) {
    win32 {
        !contains(1, ^.:.*):1 = $$OUT_PWD/$$1
    } else {
        !contains(1, ^/.*):1 = $$OUT_PWD/$$1
    }
    out = $$clean_path($$1)
    out ~= s|^$$re_escape($$SRCRESOURCEDIR)||$$i_flag
    return($$out)
}

# files that are to be unconditionally "deployed" to the build dir from src/share to share
DATA_DIRS = \
    externaltools
DATA_FILES_SRC = \
    externaltools/lrelease.xml \
    externaltools/lupdate.xml \
    externaltools/sort.xml \
    externaltools/qmlviewer.xml \
    externaltools/qmlscene.xml
unix {
    macx:DATA_FILES_SRC += externaltools/vi_mac.xml
    else:DATA_FILES_SRC += externaltools/vi.xml
} else {
    DATA_FILES_SRC += externaltools/notepad_win.xml
}
for(file, DATA_FILES_SRC):DATA_FILES += $${SRCRESOURCEDIR}$$file
unconditionalCopy2build.input = DATA_FILES
unconditionalCopy2build.output = $$IDE_DATA_PATH/${QMAKE_FUNC_FILE_IN_stripSrcResourceDir}
isEmpty(vcproj):unconditionalCopy2build.variable_out = PRE_TARGETDEPS
win32:unconditionalCopy2build.commands = $$QMAKE_COPY \"${QMAKE_FILE_IN}\" \"${QMAKE_FILE_OUT}\"
unix:unconditionalCopy2build.commands = $$QMAKE_COPY ${QMAKE_FILE_IN} ${QMAKE_FILE_OUT}
unconditionalCopy2build.name = COPY ${QMAKE_FILE_IN}
unconditionalCopy2build.CONFIG += no_link
QMAKE_EXTRA_COMPILERS += unconditionalCopy2build

!macx {
    for(data_dir, DATA_DIRS) {
        eval($${data_dir}.files = $$IDE_DATA_PATH/$$data_dir)
        eval($${data_dir}.path = $$QTC_PREFIX/share/qtcreator)
        eval($${data_dir}.CONFIG += no_check_exist)
        INSTALLS += $$data_dir
    }
}
