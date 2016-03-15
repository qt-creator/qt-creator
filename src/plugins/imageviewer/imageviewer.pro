include(../../qtcreatorplugin.pri)

HEADERS += \
    imageviewerplugin.h \
    imageviewerfactory.h \
    imageviewerfile.h \
    imageviewer.h \
    imageview.h \
    imageviewerconstants.h

SOURCES += \
    imageviewerplugin.cpp \
    imageviewerfactory.cpp \
    imageviewerfile.cpp \
    imageviewer.cpp \
    imageview.cpp

RESOURCES += \
    imageviewer.qrc

!isEmpty(QT.svg.name): QT += svg
else: DEFINES += QT_NO_SVG

FORMS += \
    imageviewertoolbar.ui
