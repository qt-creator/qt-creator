TEMPLATE = aux

include(../../../qtcreator.pri)

STATIC_BASE = $$PWD
STATIC_OUTPUT_BASE = $$IDE_DATA_PATH
STATIC_INSTALL_BASE = $$INSTALL_DATA_PATH

DATA_DIRS = \
    generic-highlighter \
    fonts

for(data_dir, DATA_DIRS) {
    STATIC_FILES += $$files($$PWD/$$data_dir/*, true)
}

include(../../../qtcreatordata.pri)
