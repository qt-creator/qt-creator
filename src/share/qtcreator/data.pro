TEMPLATE = aux

include(../../../qtcreator.pri)

STATIC_BASE = $$PWD
STATIC_OUTPUT_BASE = $$IDE_DATA_PATH
STATIC_INSTALL_BASE = $$INSTALL_DATA_PATH

STATIC_FILES = \
    $$PWD/externaltools/lrelease.xml \
    $$PWD/externaltools/lupdate.xml \
    $$PWD/externaltools/sort.xml \
    $$PWD/externaltools/qmlviewer.xml \
    $$PWD/externaltools/qmlscene.xml
unix {
    osx:STATIC_FILES += $$PWD/externaltools/vi_mac.xml
    else:STATIC_FILES += $$PWD/externaltools/vi.xml
} else {
    STATIC_FILES += $$PWD/externaltools/notepad_win.xml
}

include(../../../qtcreatordata.pri)
