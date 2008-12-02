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
    texteditoractionhandler.cpp \
    completionsupport.cpp \
    completionwidget.cpp \
    fontsettingspage.cpp \
    tabsettings.cpp \
    storagesettings.cpp \
    displaysettings.cpp \
    fontsettings.cpp \
    textblockiterator.cpp \
    linenumberfilter.cpp \
    generalsettingspage.cpp \
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
    fontsettings.h \
    textblockiterator.h \
    itexteditable.h \
    itexteditor.h \
    linenumberfilter.h \
    texteditor_global.h \
    generalsettingspage.h \
    basetextmark.h \
    findinfiles.h \
    basefilefind.h \
    texteditorsettings.h \
    codecselector.h
FORMS += fontsettingspage.ui \
    generalsettingspage.ui
RESOURCES += texteditor.qrc
