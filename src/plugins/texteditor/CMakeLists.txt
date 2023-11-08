if(TARGET KSyntaxHighlighting)
  set(KSYNTAXHIGHLIGHTING_TARGET KSyntaxHighlighting)
else()
  set(KSYNTAXHIGHLIGHTING_TARGET KF5::SyntaxHighlighting)
endif()

add_qtc_plugin(TextEditor
  DEPENDS Qt::Concurrent Qt::Network Qt::PrintSupport Qt::Xml
  PUBLIC_DEPENDS ${KSYNTAXHIGHLIGHTING_TARGET}
  PLUGIN_DEPENDS Core
  SOURCES
    autocompleter.cpp autocompleter.h
    basefilefind.cpp basefilefind.h
    basehoverhandler.cpp basehoverhandler.h
    behaviorsettings.cpp behaviorsettings.h
    behaviorsettingspage.cpp behaviorsettingspage.h
    behaviorsettingswidget.cpp behaviorsettingswidget.h
    blockrange.h
    bookmark.cpp bookmark.h
    bookmarkfilter.cpp bookmarkfilter.h
    bookmarkmanager.cpp bookmarkmanager.h
    circularclipboard.cpp circularclipboard.h
    circularclipboardassist.cpp circularclipboardassist.h
    codeassist/assistenums.h
    codeassist/assistinterface.cpp codeassist/assistinterface.h
    codeassist/assistproposalitem.cpp codeassist/assistproposalitem.h
    codeassist/assistproposaliteminterface.h
    codeassist/asyncprocessor.cpp codeassist/asyncprocessor.h
    codeassist/codeassistant.cpp codeassist/codeassistant.h
    codeassist/completionassistprovider.cpp codeassist/completionassistprovider.h
    codeassist/documentcontentcompletion.cpp codeassist/documentcontentcompletion.h
    codeassist/functionhintproposal.cpp codeassist/functionhintproposal.h
    codeassist/functionhintproposalwidget.cpp codeassist/functionhintproposalwidget.h
    codeassist/genericproposal.cpp codeassist/genericproposal.h
    codeassist/genericproposalmodel.cpp codeassist/genericproposalmodel.h
    codeassist/genericproposalwidget.cpp codeassist/genericproposalwidget.h
    codeassist/iassistprocessor.cpp codeassist/iassistprocessor.h
    codeassist/iassistproposal.cpp codeassist/iassistproposal.h
    codeassist/iassistproposalmodel.cpp codeassist/iassistproposalmodel.h
    codeassist/iassistproposalwidget.cpp codeassist/iassistproposalwidget.h
    codeassist/iassistprovider.cpp codeassist/iassistprovider.h
    codeassist/ifunctionhintproposalmodel.cpp codeassist/ifunctionhintproposalmodel.h
    codeassist/keywordscompletionassist.cpp codeassist/keywordscompletionassist.h
    codeassist/textdocumentmanipulator.cpp codeassist/textdocumentmanipulator.h
    codeassist/textdocumentmanipulatorinterface.h
    codecchooser.cpp codecchooser.h
    codestyleeditor.cpp codestyleeditor.h
    codestylepool.cpp codestylepool.h
    codestyleselectorwidget.cpp codestyleselectorwidget.h
    colorpreviewhoverhandler.cpp colorpreviewhoverhandler.h
    colorscheme.cpp colorscheme.h
    colorschemeedit.cpp colorschemeedit.h
    command.cpp command.h
    commentssettings.cpp commentssettings.h
    completionsettings.cpp completionsettings.h
    completionsettingspage.cpp completionsettingspage.h
    displaysettings.cpp displaysettings.h
    displaysettingspage.cpp displaysettingspage.h
    extraencodingsettings.cpp extraencodingsettings.h
    findincurrentfile.cpp findincurrentfile.h
    findinfiles.cpp findinfiles.h
    findinopenfiles.cpp findinopenfiles.h
    fontsettings.cpp fontsettings.h
    fontsettingspage.cpp fontsettingspage.h
    formatter.h
    formattexteditor.cpp formattexteditor.h
    highlighter.cpp highlighter.h
    highlightersettings.cpp highlightersettings.h
    highlightersettingspage.cpp highlightersettingspage.h
    icodestylepreferences.cpp icodestylepreferences.h
    icodestylepreferencesfactory.cpp icodestylepreferencesfactory.h
    indenter.h
    ioutlinewidget.h
    jsoneditor.cpp jsoneditor.h
    linenumberfilter.cpp linenumberfilter.h
    marginsettings.cpp marginsettings.h
    markdowneditor.cpp markdowneditor.h
    outlinefactory.cpp outlinefactory.h
    plaintexteditorfactory.cpp plaintexteditorfactory.h
    quickfix.cpp quickfix.h
    refactoringchanges.cpp refactoringchanges.h
    refactoroverlay.cpp refactoroverlay.h
    semantichighlighter.cpp semantichighlighter.h
    simplecodestylepreferences.cpp simplecodestylepreferences.h
    simplecodestylepreferenceswidget.cpp simplecodestylepreferenceswidget.h
    snippets/reuse.h
    snippets/snippet.cpp snippets/snippet.h
    snippets/snippetassistcollector.cpp snippets/snippetassistcollector.h
    snippets/snippeteditor.cpp snippets/snippeteditor.h
    snippets/snippetoverlay.cpp snippets/snippetoverlay.h
    snippets/snippetparser.cpp snippets/snippetparser.h
    snippets/snippetprovider.cpp snippets/snippetprovider.h
    snippets/snippetscollection.cpp snippets/snippetscollection.h
    snippets/snippetssettings.cpp snippets/snippetssettings.h
    snippets/snippetssettingspage.cpp snippets/snippetssettingspage.h
    storagesettings.cpp storagesettings.h
    syntaxhighlighter.cpp syntaxhighlighter.h
    tabsettings.cpp tabsettings.h
    tabsettingswidget.cpp tabsettingswidget.h
    textdocument.cpp textdocument.h
    textdocumentlayout.cpp textdocumentlayout.h
    texteditor.cpp texteditor.h
    texteditor.qrc
    texteditor_global.h
    texteditortr.h
    texteditoractionhandler.cpp texteditoractionhandler.h
    texteditorconstants.cpp texteditorconstants.h
    texteditoroverlay.cpp texteditoroverlay.h
    texteditorplugin.cpp texteditorplugin.h
    texteditorsettings.cpp texteditorsettings.h
    textindenter.cpp textindenter.h
    textmark.cpp textmark.h
    textstyles.h
    typingsettings.cpp typingsettings.h
)

extend_qtc_plugin(TextEditor
  CONDITION WITH_TESTS
  SOURCES
    codeassist/codeassist_test.cpp codeassist/codeassist_test.h
    highlighter_test.cpp highlighter_test.h
    texteditor_test.cpp
)
