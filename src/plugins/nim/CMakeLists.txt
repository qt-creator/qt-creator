add_qtc_plugin(Nim
  PLUGIN_DEPENDS Core TextEditor ProjectExplorer
  SOURCES
    editor/nimcompletionassistprovider.cpp editor/nimcompletionassistprovider.h
    editor/nimeditorfactory.cpp editor/nimeditorfactory.h
    editor/nimhighlighter.cpp editor/nimhighlighter.h
    editor/nimindenter.cpp editor/nimindenter.h
    editor/nimtexteditorwidget.cpp editor/nimtexteditorwidget.h
    nim.qrc
    nimconstants.h
    nimplugin.cpp nimplugin.h
    project/nimblebuildstep.h project/nimblebuildstep.cpp
    project/nimbleproject.h project/nimbleproject.cpp
    project/nimblerunconfiguration.h project/nimblerunconfiguration.cpp
    project/nimbletaskstep.h project/nimbletaskstep.cpp
    project/nimbletaskstepwidget.h project/nimbletaskstepwidget.cpp project/nimbletaskstepwidget.ui
    project/nimblebuildsystem.h project/nimblebuildsystem.cpp
    project/nimblebuildconfiguration.h project/nimblebuildconfiguration.cpp
    project/nimbuildsystem.cpp project/nimbuildsystem.h
    project/nimbuildconfiguration.cpp project/nimbuildconfiguration.h
    project/nimcompilerbuildstep.cpp project/nimcompilerbuildstep.h
    project/nimcompilerbuildstepconfigwidget.cpp project/nimcompilerbuildstepconfigwidget.h project/nimcompilerbuildstepconfigwidget.ui
    project/nimcompilercleanstep.cpp project/nimcompilercleanstep.h
    project/nimproject.cpp project/nimproject.h
    project/nimprojectnode.cpp project/nimprojectnode.h
    project/nimrunconfiguration.cpp project/nimrunconfiguration.h
    project/nimtoolchain.cpp project/nimtoolchain.h
    project/nimtoolchainfactory.cpp project/nimtoolchainfactory.h
    settings/nimcodestylepreferencesfactory.cpp settings/nimcodestylepreferencesfactory.h
    settings/nimcodestylepreferenceswidget.cpp settings/nimcodestylepreferenceswidget.h settings/nimcodestylepreferenceswidget.ui
    settings/nimcodestylesettingspage.cpp settings/nimcodestylesettingspage.h
    settings/nimsettings.cpp settings/nimsettings.h
    settings/nimtoolssettingspage.cpp settings/nimtoolssettingspage.h
    settings/nimtoolssettingswidget.ui
    suggest/client.cpp suggest/client.h
    suggest/clientrequests.cpp suggest/clientrequests.h
    suggest/nimsuggest.cpp suggest/nimsuggest.h
    suggest/nimsuggestcache.cpp suggest/nimsuggestcache.h
    suggest/server.cpp suggest/server.h
    suggest/sexprlexer.h suggest/sexprparser.h
    tools/nimlexer.cpp tools/nimlexer.h
    tools/sourcecodestream.h
)
