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
    ImageViewer.pluginspec \
    ImageViewer.mimetypes.xml

contains(QT_CONFIG, svg):QT += svg

FORMS += \
    imageviewertoolbar.ui
