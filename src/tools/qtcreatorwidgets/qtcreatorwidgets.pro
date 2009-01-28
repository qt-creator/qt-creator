CONFIG      += designer plugin debug_and_release
TARGET      = $$qtLibraryTarget(qtcreatorwidgets)
TEMPLATE    = lib

HEADERS     = customwidgets.h \
              customwidget.h

SOURCES     = customwidgets.cpp 

# Link against the qtcreator utils lib

linux-* {
  # form abs path to qtcreator lib dir
  GH_LIB=$$dirname(PWD)
  GH_LIB=$$dirname(GH_LIB)
  GH_LIB=$$dirname(GH_LIB)
  GH_LIB=$$GH_LIB/lib
  QMAKE_RPATHDIR *= $$GH_LIB
}

INCLUDEPATH += ../../../src/libs
macx {
    LIBS += -L../../../bin/QtCreator.app/Contents/PlugIns
    CONFIG(debug, debug|release):LIBS += -lUtils_debug
    else:LIBS += -lUtils
} else {
    LIBS += -L../../../lib -lUtils
}

DESTDIR= $$[QT_INSTALL_PLUGINS]/designer

