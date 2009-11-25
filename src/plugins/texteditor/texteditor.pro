TEMPLATE = lib
TARGET = TextEditor
DEFINES += TEXTEDITOR_LIBRARY
include(../../qtcreatorplugin.pri)
include(texteditor_dependencies.pri)
SOURCES += texteditorplugin.cpp \
    textfilewizard.cpp \
    plaintexteditor.cpp \
    plaintexteditorfactory.cpp \
    basetextdocument.cpp \
    basetexteditor.cpp \
    behaviorsettingspage.cpp \
    texteditoractionhandler.cpp \
    completionsupport.cpp \
    completionwidget.cpp \
    fontsettingspage.cpp \
    tabsettings.cpp \
    storagesettings.cpp \
    displaysettings.cpp \
    displaysettingspage.cpp \
    fontsettings.cpp \
    textblockiterator.cpp \
    linenumberfilter.cpp \
    basetextmark.cpp \
    findinfiles.cpp \
    basefilefind.cpp \
    texteditorsettings.cpp \
    codecselector.cpp \
    findincurrentfile.cpp \
    colorscheme.cpp \
    colorschemeedit.cpp \
    itexteditor.cpp \
    texteditoroverlay.cpp

HEADERS += texteditorplugin.h \
    textfilewizard.h \
    plaintexteditor.h \
    plaintexteditorfactory.h \
    basetexteditor_p.h \
    basetextdocument.h \
    behaviorsettingspage.h \
    completionsupport.h \
    completionwidget.h \
    basetexteditor.h \
    texteditoractionhandler.h \
    fontsettingspage.h \
    icompletioncollector.h \
    texteditorconstants.h \
    tabsettings.h \
    storagesettings.h \
    displaysettings.h \
    displaysettingspage.h \
    fontsettings.h \
    textblockiterator.h \
    itexteditable.h \
    itexteditor.h \
    linenumberfilter.h \
    texteditor_global.h \
    basetextmark.h \
    findinfiles.h \
    basefilefind.h \
    texteditorsettings.h \
    codecselector.h \
    findincurrentfile.h \
    colorscheme.h \
    colorschemeedit.h \
    texteditoroverlay.h


FORMS += behaviorsettingspage.ui \
    displaysettingspage.ui \
    fontsettingspage.ui \
    colorschemeedit.ui
RESOURCES += texteditor.qrc
OTHER_FILES += TextEditor.pluginspec
