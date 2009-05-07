include(../qtcreator.pri)

TEMPLATE = app
TARGET = $$IDE_APP_WRAPPER
OBJECTS_DIR =

PRE_TARGETDEPS = $$PWD/qtcreator

QMAKE_LINK = cp $$PWD/qtcreator $@ && : IGNORE REST

QMAKE_CLEAN = $$IDE_APP_WRAPPER

target.path  = /bin
INSTALLS    += target
