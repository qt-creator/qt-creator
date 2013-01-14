TEMPLATE = lib
TARGET = PythonEditor

include(../../qtcreatorplugin.pri)

# dependencies
include(../../plugins/coreplugin/coreplugin.pri)
include(../../plugins/texteditor/texteditor.pri)
include(../../plugins/cpptools/cpptools.pri)

DEFINES += \
    PYEDITOR_LIBRARY

OTHER_FILES += PythonEditor.pluginspec.in \
    pythoneditor.mimetypes.xml

RESOURCES += \
    pythoneditorplugin.qrc

HEADERS += \
    pythoneditor_global.h \
    pythoneditorplugin.h \
    pythoneditorfactory.h \
    pythoneditor.h \
    pythoneditorwidget.h \
    pythoneditorconstants.h \
    wizard/pythonfilewizard.h \
    tools/pythonhighlighter.h \
    tools/pythonindenter.h \
    tools/lexical/pythonformattoken.h \
    tools/lexical/pythonscanner.h \
    tools/lexical/sourcecodestream.h

SOURCES += \
    pythoneditorplugin.cpp \
    pythoneditorfactory.cpp \
    pythoneditor.cpp \
    pythoneditorwidget.cpp \
    wizard/pythonfilewizard.cpp \
    tools/pythonhighlighter.cpp \
    tools/pythonindenter.cpp \
    tools/lexical/pythonscanner.cpp
