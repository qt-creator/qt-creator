CONFIG      += designer plugin debug_and_release
TARGET      = $$qtLibraryTarget(qtcreatorwidgets)
TEMPLATE    = lib

HEADERS     = customwidgets.h \
              customwidget.h

SOURCES     = customwidgets.cpp

# Link against the qtcreator utils lib

isEmpty(IDE_LIBRARY_BASENAME) {
    IDE_LIBRARY_BASENAME = lib
}

linux-* {
  # form abs path to qtcreator lib dir
  QTC_LIBS=$$dirname(PWD)
  QTC_LIBS=$$dirname(QTC_LIBS)
  QTC_LIBS=$$dirname(QTC_LIBS)
  QTC_LIBS=$$QTC_LIBS/$$IDE_LIBRARY_BASENAME/qtcreator
  QMAKE_RPATHDIR *= $$QTC_LIBS
}

INCLUDEPATH += ../../../src/libs
macx {
    LIBS += -L../../../bin/QtCreator.app/Contents/PlugIns
    CONFIG(debug, debug|release):LIBS += -lUtils_debug
    else:LIBS += -lUtils
} else {
    message($$QTC_LIBS)
    LIBS += -L$$QTC_LIBS -lUtils
}

DESTDIR= $$[QT_INSTALL_PLUGINS]/designer

