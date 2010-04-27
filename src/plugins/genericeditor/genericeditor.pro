TEMPLATE = lib
TARGET = GenericEditor
include(../../qtcreatorplugin.pri)
include(genericeditor_dependencies.pri)

CONFIG += help

HEADERS += \
    genericeditorplugin.h \
    progressdata.h \
    genericeditorfactory.h \
    genericeditor.h \
    languagespecificfactories.h \
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
    genericeditorconstants.h

SOURCES += \
    genericeditorplugin.cpp \
    progressdata.cpp \
    genericeditorfactory.cpp \
    genericeditor.cpp \
    languagespecificfactories.cpp \
    specificrules.cpp \
    rule.cpp \
    keywordlist.cpp \
    itemdata.cpp \
    includerulesinstruction.cpp \
    highlighter.cpp \
    highlightdefinitionhandler.cpp \
    highlightdefinition.cpp \
    dynamicrule.cpp \
    context.cpp

OTHER_FILES += GenericEditor.pluginspec GenericEditor.mimetypes.xml

RESOURCES += \
    genericeditor.qrc
