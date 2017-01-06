TEMPLATE = aux

include(../../qtcreator.pri)

STATIC_BASE = $$PWD
STATIC_OUTPUT_BASE = $$IDE_DATA_PATH
STATIC_INSTALL_BASE = $$INSTALL_DATA_PATH

DATA_DIRS = \
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
    modeleditor \
    glsl \
    cplusplus
macx: DATA_DIRS += scripts

for(data_dir, DATA_DIRS) {
    files = $$files($$PWD/$$data_dir/*, true)
    # Info.plist.in are handled below
    for(file, files):!contains(file, ".*/Info\\.plist\\.in$"):!exists($$file/*): \
        STATIC_FILES += $$file
}

include(../../qtcreatordata.pri)
