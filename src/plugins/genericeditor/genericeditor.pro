TEMPLATE = lib
TARGET = GenericEditor
include(../../qtcreatorplugin.pri)
include(genericeditor_dependencies.pri)

CONFIG += help

HEADERS += \
    genericeditorplugin.h \
    progressdata.h \
    specificrules.h \
    rule.h \
    reuse.h \
    keywordlist.h \
    itemdata.h \
    includerulesinstruction.h \
    highlighterexception.h \
    highlighter.h \
    highlightdefinitionhandler.h \
    highlightdefinition.h \
    dynamicrule.h \
    context.h \
    genericeditorconstants.h \
    editor.h \
    editorfactory.h

SOURCES += \
    genericeditorplugin.cpp \
    progressdata.cpp \
    specificrules.cpp \
    rule.cpp \
    keywordlist.cpp \
    itemdata.cpp \
    includerulesinstruction.cpp \
    highlighter.cpp \
    highlightdefinitionhandler.cpp \
    highlightdefinition.cpp \
    dynamicrule.cpp \
    context.cpp \
    editor.cpp \
    editorfactory.cpp

OTHER_FILES += GenericEditor.pluginspec GenericEditor.mimetypes.xml

RESOURCES += \
    genericeditor.qrc
