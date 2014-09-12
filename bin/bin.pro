TEMPLATE = app
TARGET = qtcreator.sh
OBJECTS_DIR =

PRE_TARGETDEPS = $$PWD/qtcreator.sh

QMAKE_LINK = cp $$PWD/qtcreator.sh $@ && : IGNORE REST OF LINE:
QMAKE_STRIP =
CONFIG -= qt separate_debug_info gdb_dwarf_index

QMAKE_CLEAN = qtcreator.sh

target.path  = $$QTC_PREFIX/bin
INSTALLS    += target

DISTFILES = $$PWD/qtcreator.sh
