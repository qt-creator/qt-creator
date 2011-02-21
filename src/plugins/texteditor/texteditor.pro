TEMPLATE = lib
TARGET = TextEditor
DEFINES += TEXTEDITOR_LIBRARY
QT += xml network
include(../../qtcreatorplugin.pri)
include(texteditor_dependencies.pri)
INCLUDEPATH += generichighlighter \
    tooltip \
    snippets
DEPENDPATH += generichighlighter \
    tooltip \
    snippets
SOURCES += texteditorplugin.cpp \
    textfilewizard.cpp \
    plaintexteditor.cpp \
    plaintexteditorfactory.cpp \
    basetextdocument.cpp \
    basetexteditor.cpp \
    behaviorsettings.cpp \
    behaviorsettingspage.cpp \
    texteditoractionhandler.cpp \
    icompletioncollector.cpp \
    completionsupport.cpp \
    completionwidget.cpp \
    fontsettingspage.cpp \
    tabsettings.cpp \
    storagesettings.cpp \
    displaysettings.cpp \
    displaysettingspage.cpp \
    fontsettings.cpp \
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
    texteditoroverlay.cpp \
    texteditoroptionspage.cpp \
    basetextdocumentlayout.cpp \
    completionsettings.cpp \
    normalindenter.cpp \
    indenter.cpp \
    quickfix.cpp \
    syntaxhighlighter.cpp \
    generichighlighter/itemdata.cpp \
    generichighlighter/specificrules.cpp \
    generichighlighter/rule.cpp \
    generichighlighter/dynamicrule.cpp \
    generichighlighter/context.cpp \
    generichighlighter/includerulesinstruction.cpp \
    generichighlighter/progressdata.cpp \
    generichighlighter/keywordlist.cpp \
    generichighlighter/highlightdefinition.cpp \
    generichighlighter/highlighter.cpp \
    generichighlighter/manager.cpp \
    generichighlighter/highlightdefinitionhandler.cpp \
    generichighlighter/highlightersettingspage.cpp \
    generichighlighter/highlightersettings.cpp \
    generichighlighter/managedefinitionsdialog.cpp \
    generichighlighter/highlightdefinitionmetadata.cpp \
    generichighlighter/definitiondownloader.cpp \
    refactoringchanges.cpp \
    refactoroverlay.cpp \
    outlinefactory.cpp \
    tooltip/tooltip.cpp \
    tooltip/tips.cpp \
    tooltip/tipcontents.cpp \
    tooltip/tipfactory.cpp \
    basehoverhandler.cpp \
    helpitem.cpp \
    autocompleter.cpp \
    snippets/snippetssettingspage.cpp \
    snippets/snippet.cpp \
    snippets/snippeteditor.cpp \
    snippets/snippetscollection.cpp \
    snippets/snippetssettings.cpp \
    snippets/isnippetprovider.cpp \
    snippets/snippetcollector.cpp \
    snippets/plaintextsnippetprovider.cpp \
    behaviorsettingswidget.cpp \
    extraencodingsettings.cpp

HEADERS += texteditorplugin.h \
    textfilewizard.h \
    plaintexteditor.h \
    plaintexteditorfactory.h \
    basetexteditor_p.h \
    basetextdocument.h \
    behaviorsettings.h \
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
    texteditoroverlay.h \
    texteditoroptionspage.h \
    basetextdocumentlayout.h \
    completionsettings.h \
    normalindenter.h \
    indenter.h \
    quickfix.h \
    syntaxhighlighter.h \
    generichighlighter/reuse.h \
    generichighlighter/itemdata.h \
    generichighlighter/specificrules.h \
    generichighlighter/rule.h \
    generichighlighter/reuse.h \
    generichighlighter/dynamicrule.h \
    generichighlighter/context.h \
    generichighlighter/includerulesinstruction.h \
    generichighlighter/progressdata.h \
    generichighlighter/keywordlist.h \
    generichighlighter/highlighterexception.h \
    generichighlighter/highlightdefinition.h \
    generichighlighter/highlighter.h \
    generichighlighter/manager.h \
    generichighlighter/highlightdefinitionhandler.h \
    generichighlighter/highlightersettingspage.h \
    generichighlighter/highlightersettings.h \
    generichighlighter/managedefinitionsdialog.h \
    generichighlighter/highlightdefinitionmetadata.h \
    generichighlighter/definitiondownloader.h \
    refactoringchanges.h \
    refactoroverlay.h \
    outlinefactory.h \
    ioutlinewidget.h \
    tooltip/tooltip.h \
    tooltip/tips.h \
    tooltip/tipcontents.h \
    tooltip/reuse.h \
    tooltip/effects.h \
    tooltip/tipfactory.h \
    basehoverhandler.h \
    helpitem.h \
    autocompleter.h \
    snippets/snippetssettingspage.h \
    snippets/snippet.h \
    snippets/snippeteditor.h \
    snippets/snippetscollection.h \
    snippets/reuse.h \
    snippets/snippetssettings.h \
    snippets/isnippetprovider.h \
    snippets/snippetcollector.h \
    snippets/plaintextsnippetprovider.h \
    behaviorsettingswidget.h \
    extraencodingsettings.h

FORMS += \
    displaysettingspage.ui \
    fontsettingspage.ui \
    colorschemeedit.ui \
    generichighlighter/highlightersettingspage.ui \
    generichighlighter/managedefinitionsdialog.ui \
    snippets/snippetssettingspage.ui \
    behaviorsettingswidget.ui \
    behaviorsettingspage.ui
RESOURCES += texteditor.qrc
OTHER_FILES += TextEditor.mimetypes.xml
