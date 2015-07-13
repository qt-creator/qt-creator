TEMPLATE = aux

STATIC_BASE = $$PWD
STATIC_DIRS = templates

for(data_dir, STATIC_DIRS) {
    files = $$files($$STATIC_BASE/$$data_dir/*, true)
    for(file, files): !exists($$file/*): STATIC_FILES += $$file
}

include(../../qtcreator/share/qtcreator/static.pri)

