TEMPLATE = lib
TARGET = BinEditor

include(bineditor_dependencies.pri)

HEADERS += bineditorplugin.h \
        bineditor.h \
        bineditorconstants.h \
    imageviewer.h

SOURCES += bineditorplugin.cpp \
        bineditor.cpp \
    imageviewer.cpp

RESOURCES += bineditor.qrc

OTHER_FILES += BinEditor.pluginspec BinEditor.mimetypes.xml \
    ImageViewer.mimetypes.xml
