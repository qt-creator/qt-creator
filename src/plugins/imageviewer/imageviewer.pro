TEMPLATE = lib
TARGET = ImageViewer
include(../../qtcreatorplugin.pri)
include(imageviewer_dependencies.pri)

HEADERS += \
    imageviewerplugin.h \
    imageviewerfactory.h \
    imageviewerfile.h \
    imageviewer.h \
    imageview.h \
    imageviewerconstants.h \
    imagevieweractionhandler.h

SOURCES += \
    imageviewerplugin.cpp \
    imageviewerfactory.cpp \
    imageviewerfile.cpp \
    imageviewer.cpp \
    imageview.cpp \
    imagevieweractionhandler.cpp

RESOURCES += \
    imageviewer.qrc

OTHER_FILES += \
    ImageViewer.mimetypes.xml

greaterThan(QT_MAJOR_VERSION, 4) {
    !isEmpty(QT.svg.name): QT += svg
    else: DEFINES += QT_NO_SVG
} else {
    contains(QT_CONFIG, svg): QT += svg
}

FORMS += \
    imageviewertoolbar.ui
