add_qtc_plugin(ClangRefactoring
  BUILD_DEFAULT OFF
  CONDITION TARGET libclang
  DEPENDS ClangSupport CPlusPlus
  PLUGIN_DEPENDS Core CppTools TextEditor ClangPchManager
  SOURCES ${TEST_SOURCES}
    baseclangquerytexteditorwidget.cpp baseclangquerytexteditorwidget.h
    clangqueryexamplehighlighter.cpp clangqueryexamplehighlighter.h
    clangqueryexamplehighlightmarker.h
    clangqueryexampletexteditorwidget.cpp clangqueryexampletexteditorwidget.h
    clangqueryhighlighter.cpp clangqueryhighlighter.h
    clangqueryhighlightmarker.h
    clangqueryhoverhandler.cpp clangqueryhoverhandler.h
    clangqueryprojectsfindfilter.cpp clangqueryprojectsfindfilter.h clangqueryprojectsfindfilter.ui
    clangqueryprojectsfindfilterwidget.cpp clangqueryprojectsfindfilterwidget.h
    clangquerytexteditorwidget.cpp clangquerytexteditorwidget.h
    clangrefactoringplugin.cpp clangrefactoringplugin.h
    editormanagerinterface.h
    locatorfilter.cpp locatorfilter.h
    projectpartproviderinterface.h
    projectpartutilities.cpp projectpartutilities.h
    qtcreatorclangqueryfindfilter.cpp qtcreatorclangqueryfindfilter.h
    qtcreatoreditormanager.cpp qtcreatoreditormanager.h
    qtcreatorrefactoringprojectupdater.cpp qtcreatorrefactoringprojectupdater.h
    qtcreatorsearch.cpp qtcreatorsearch.h
    qtcreatorsearchhandle.cpp qtcreatorsearchhandle.h
    qtcreatorsymbolsfindfilter.cpp qtcreatorsymbolsfindfilter.h
    querysqlitestatementfactory.h
    refactoringclient.cpp refactoringclient.h
    refactoringconnectionclient.cpp refactoringconnectionclient.h
    refactoringengine.cpp refactoringengine.h
    refactoringprojectupdater.cpp refactoringprojectupdater.h
    searchhandle.cpp searchhandle.h
    searchinterface.h
    sourcelocations.h
    symbol.h
    symbolquery.h
    symbolqueryinterface.h
    clangsymbolsfindfilter.cpp clangsymbolsfindfilter.h
    symbolsfindfilterconfigwidget.cpp symbolsfindfilterconfigwidget.h
)
