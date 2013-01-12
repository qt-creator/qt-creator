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
glslfilewizard.h \
glslhighlighter.h \
glslautocompleter.h \
glslindenter.h \
glslhoverhandler.h \
    glslcompletionassist.h \
    reuse.h

SOURCES += \
glsleditor.cpp \
glsleditoractionhandler.cpp \
glsleditoreditable.cpp \
glsleditorfactory.cpp \
glsleditorplugin.cpp \
glslfilewizard.cpp \
glslhighlighter.cpp \
glslautocompleter.cpp \
glslindenter.cpp \
glslhoverhandler.cpp \
    glslcompletionassist.cpp \
    reuse.cpp

OTHER_FILES += GLSLEditor.mimetypes.xml
RESOURCES += glsleditor.qrc
