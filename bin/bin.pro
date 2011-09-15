TEMPLATE = app
TARGET = qtcreator.sh
OBJECTS_DIR =

PRE_TARGETDEPS = $$PWD/qtcreator.sh

QMAKE_LINK = cp $$PWD/qtcreator.sh $@ && : IGNORE REST OF LINE:
QMAKE_STRIP =

QMAKE_CLEAN = qtcreator.sh

target.path  = /bin
INSTALLS    += target

OTHER_FILES = $$PWD/qtcreator.sh
