TEMPLATE = lib
TARGET = GLSLEditor
include(../../qtcreatorplugin.pri)
include(glsleditor_dependencies.pri)

DEFINES += \
    GLSLEDITOR_LIBRARY \
    QT_CREATOR

HEADERS += \
glsleditor.h \
glsleditor_global.h \
glsleditoractionhandler.h \
glsleditorconstants.h \
glsleditoreditable.h \
glsleditorfactory.h \
glsleditorplugin.h \
    glslhighlighter.h

SOURCES += \
glsleditor.cpp \
glsleditoractionhandler.cpp \
glsleditoreditable.cpp \
glsleditorfactory.cpp \
glsleditorplugin.cpp \
    glslhighlighter.cpp

OTHER_FILES += GLSLEditor.mimetypes.xml
RESOURCES += glsleditor.qrc
