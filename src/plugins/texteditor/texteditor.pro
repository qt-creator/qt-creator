TEMPLATE = lib
TARGET = TextEditor
DEFINES += TEXTEDITOR_LIBRARY
include(../../qworkbenchplugin.pri)
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
    interactionsettings.cpp \
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
    codecselector.cpp
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
    interactionsettings.h \
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
    codecselector.h
FORMS += behaviorsettingspage.ui \
    displaysettingspage.ui \
    fontsettingspage.ui
RESOURCES += texteditor.qrc
