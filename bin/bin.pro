TEMPLATE = app
TARGET = qtcreator.sh

include(../qtcreator.pri)

OBJECTS_DIR =

PRE_TARGETDEPS = $$PWD/qtcreator.sh

QMAKE_LINK = cp $$PWD/qtcreator.sh $@ && : IGNORE REST OF LINE:
QMAKE_STRIP =
CONFIG -= qt separate_debug_info gdb_dwarf_index

QMAKE_CLEAN = qtcreator.sh

target.path  = $$INSTALL_BIN_PATH
INSTALLS    += target

DISTFILES = $$PWD/qtcreator.sh
