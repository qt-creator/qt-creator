TEMPLATE = lib
TARGET = ImageViewer

include(imageviewer_dependencies.pri)

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

OTHER_FILES += \
    ImageViewer.pluginspec \
    ImageViewer.mimetypes.xml

QT += svg

FORMS += \
    imageviewertoolbar.ui
