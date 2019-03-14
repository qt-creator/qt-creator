DEFINES += TEXTEDITOR_LIBRARY
QT += gui-private network printsupport xml
CONFIG += exceptions
CONFIG += include_source_dir # For the highlighter autotest.

include(../../shared/syntax/syntax_shared.pri)
isEmpty(KSYNTAXHIGHLIGHTING_LIB_DIR) | isEmpty(KSYNTAXHIGHLIGHTING_INCLUDE_DIR) {
    QTC_LIB_DEPENDS += syntax-highlighting
} else {
    unix:!disable_external_rpath {
        !macos: QMAKE_LFLAGS += -Wl,-z,origin
        QMAKE_LFLAGS += -Wl,-rpath,$$shell_quote($${KSYNTAXHIGHLIGHTING_LIB_DIR})
    }
}

include(../../qtcreatorplugin.pri)

SOURCES += texteditorplugin.cpp \
    plaintexteditorfactory.cpp \
    textdocument.cpp \
    texteditor.cpp \
    behaviorsettings.cpp \
    behaviorsettingspage.cpp \
    texteditoractionhandler.cpp \
    fontsettingspage.cpp \
    texteditorconstants.cpp \
    tabsettings.cpp \
    storagesettings.cpp \
    displaysettings.cpp \
    displaysettingspage.cpp \
    fontsettings.cpp \
    linenumberfilter.cpp \
    findinfiles.cpp \
    basefilefind.cpp \
    texteditorsettings.cpp \
    codecselector.cpp \
    findincurrentfile.cpp \
    findinopenfiles.cpp \
    colorscheme.cpp \
    colorschemeedit.cpp \
    texteditoroverlay.cpp \
    texteditoroptionspage.cpp \
    textdocumentlayout.cpp \
    completionsettings.cpp \
    normalindenter.cpp \
    textindenter.cpp \
    quickfix.cpp \
    syntaxhighlighter.cpp \
    highlighter.cpp \
    highlightersettings.cpp \
    highlightersettingspage.cpp \
    refactoringchanges.cpp \
    refactoroverlay.cpp \
    outlinefactory.cpp \
    basehoverhandler.cpp \
    colorpreviewhoverhandler.cpp \
    autocompleter.cpp \
    snippets/snippetssettingspage.cpp \
    snippets/snippet.cpp \
    snippets/snippeteditor.cpp \
    snippets/snippetscollection.cpp \
    snippets/snippetssettings.cpp \
    snippets/snippetprovider.cpp \
    behaviorsettingswidget.cpp \
    extraencodingsettings.cpp \
    codeassist/functionhintproposalwidget.cpp \
    codeassist/ifunctionhintproposalmodel.cpp \
    codeassist/functionhintproposal.cpp \
    codeassist/iassistprovider.cpp \
    codeassist/iassistproposal.cpp \
    codeassist/iassistprocessor.cpp \
    codeassist/iassistproposalwidget.cpp \
    codeassist/codeassistant.cpp \
    snippets/snippetassistcollector.cpp \
    codeassist/assistinterface.cpp \
    codeassist/assistproposalitem.cpp \
    codeassist/runner.cpp \
    codeassist/completionassistprovider.cpp \
    codeassist/genericproposalmodel.cpp \
    codeassist/genericproposal.cpp \
    codeassist/genericproposalwidget.cpp \
    codeassist/iassistproposalmodel.cpp \
    codeassist/textdocumentmanipulator.cpp \
    codeassist/documentcontentcompletion.cpp\
    tabsettingswidget.cpp \
    simplecodestylepreferences.cpp \
    simplecodestylepreferenceswidget.cpp \
    icodestylepreferencesfactory.cpp \
    semantichighlighter.cpp \
    codestyleselectorwidget.cpp \
    typingsettings.cpp \
    icodestylepreferences.cpp \
    codestylepool.cpp \
    codestyleeditor.cpp \
    circularclipboard.cpp \
    circularclipboardassist.cpp \
    textmark.cpp \
    codeassist/keywordscompletionassist.cpp \
    completionsettingspage.cpp \
    commentssettings.cpp \
    marginsettings.cpp \
    formattexteditor.cpp \
    command.cpp

HEADERS += texteditorplugin.h \
    plaintexteditorfactory.h \
    texteditor_p.h \
    textdocument.h \
    behaviorsettings.h \
    behaviorsettingspage.h \
    texteditor.h \
    texteditoractionhandler.h \
    fontsettingspage.h \
    texteditorconstants.h \
    tabsettings.h \
    storagesettings.h \
    displaysettings.h \
    displaysettingspage.h \
    fontsettings.h \
    linenumberfilter.h \
    texteditor_global.h \
    findinfiles.h \
    basefilefind.h \
    texteditorsettings.h \
    codecselector.h \
    findincurrentfile.h \
    findinopenfiles.h \
    colorscheme.h \
    colorschemeedit.h \
    texteditoroverlay.h \
    texteditoroptionspage.h \
    textdocumentlayout.h \
    completionsettings.h \
    normalindenter.h \
    textindenter.h \
    quickfix.h \
    syntaxhighlighter.h \
    highlighter.h \
    highlightersettings.h \
    highlightersettingspage.h \
    refactoringchanges.h \
    refactoroverlay.h \
    outlinefactory.h \
    ioutlinewidget.h \
    basehoverhandler.h \
    colorpreviewhoverhandler.h \
    autocompleter.h \
    snippets/snippetssettingspage.h \
    snippets/snippet.h \
    snippets/snippeteditor.h \
    snippets/snippetscollection.h \
    snippets/reuse.h \
    snippets/snippetssettings.h \
    snippets/snippetprovider.h \
    behaviorsettingswidget.h \
    extraencodingsettings.h \
    codeassist/functionhintproposalwidget.h \
    codeassist/ifunctionhintproposalmodel.h \
    codeassist/functionhintproposal.h \
    codeassist/iassistprovider.h \
    codeassist/iassistprocessor.h \
    codeassist/iassistproposalwidget.h \
    codeassist/iassistproposal.h \
    codeassist/codeassistant.h \
    snippets/snippetassistcollector.h \
    codeassist/assistinterface.h \
    codeassist/assistproposalitem.h \
    codeassist/assistenums.h \
    codeassist/runner.h \
    codeassist/assistproposaliteminterface.h \
    codeassist/completionassistprovider.h \
    codeassist/genericproposalmodel.h \
    codeassist/genericproposal.h \
    codeassist/genericproposalwidget.h \
    codeassist/iassistproposalmodel.h \
    codeassist/textdocumentmanipulator.h \
    codeassist/textdocumentmanipulatorinterface.h \
    codeassist/documentcontentcompletion.h \
    tabsettingswidget.h \
    simplecodestylepreferences.h \
    simplecodestylepreferenceswidget.h \
    icodestylepreferencesfactory.h \
    semantichighlighter.h \
    codestyleselectorwidget.h \
    typingsettings.h \
    icodestylepreferences.h \
    codestylepool.h \
    codestyleeditor.h \
    circularclipboard.h \
    circularclipboardassist.h \
    textmark.h \
    codeassist/keywordscompletionassist.h \
    marginsettings.h \
    blockrange.h \
    completionsettingspage.h \
    commentssettings.h \
    textstyles.h \
    formattexteditor.h \
    command.h \
    indenter.h

FORMS += \
    displaysettingspage.ui \
    fontsettingspage.ui \
    colorschemeedit.ui \
    snippets/snippetssettingspage.ui \
    behaviorsettingswidget.ui \
    behaviorsettingspage.ui \
    tabsettingswidget.ui \
    completionsettingspage.ui \
    codestyleselectorwidget.ui \
    highlightersettingspage.ui

RESOURCES += texteditor.qrc

equals(TEST, 1) {
SOURCES += texteditor_test.cpp
}

