if (WITH_TESTS)
  set(TST_COMPONENT CppEditor QmakeProjectManager)
endif()

add_qtc_plugin(ClangCodeModel
  CONDITION TARGET libclang
  DEPENDS ClangSupport CPlusPlus
  PLUGIN_DEPENDS Core CppTools TextEditor ${TST_COMPONENT}
  SOURCES
    clangactivationsequencecontextprocessor.cpp clangactivationsequencecontextprocessor.h
    clangactivationsequenceprocessor.cpp clangactivationsequenceprocessor.h
    clangassistproposalitem.cpp clangassistproposalitem.h
    clangassistproposalmodel.cpp clangassistproposalmodel.h
    clangbackendcommunicator.cpp clangbackendcommunicator.h
    clangbackendlogging.cpp clangbackendlogging.h
    clangbackendreceiver.cpp clangbackendreceiver.h
    clangbackendsender.cpp clangbackendsender.h
    clangcodemodelplugin.cpp clangcodemodelplugin.h
    clangcompletionassistinterface.cpp clangcompletionassistinterface.h
    clangcompletionassistprocessor.cpp clangcompletionassistprocessor.h
    clangcompletionassistprovider.cpp clangcompletionassistprovider.h
    clangcompletionchunkstotextconverter.cpp clangcompletionchunkstotextconverter.h
    clangcompletioncontextanalyzer.cpp clangcompletioncontextanalyzer.h
    clangconstants.h
    clangcurrentdocumentfilter.cpp clangcurrentdocumentfilter.h
    clangdiagnosticfilter.cpp clangdiagnosticfilter.h
    clangdiagnosticmanager.cpp clangdiagnosticmanager.h
    clangdiagnostictooltipwidget.cpp clangdiagnostictooltipwidget.h
    clangeditordocumentparser.cpp clangeditordocumentparser.h
    clangeditordocumentprocessor.cpp clangeditordocumentprocessor.h
    clangfixitoperation.cpp clangfixitoperation.h
    clangfixitoperationsextractor.cpp clangfixitoperationsextractor.h
    clangfollowsymbol.cpp clangfollowsymbol.h
    clangfunctionhintmodel.cpp clangfunctionhintmodel.h
    clanghighlightingresultreporter.cpp clanghighlightingresultreporter.h
    clanghoverhandler.cpp clanghoverhandler.h
    clangisdiagnosticrelatedtolocation.h
    clangmodelmanagersupport.cpp clangmodelmanagersupport.h
    clangoverviewmodel.cpp clangoverviewmodel.h
    clangpreprocessorassistproposalitem.cpp clangpreprocessorassistproposalitem.h
    clangprojectsettings.cpp clangprojectsettings.h
    clangprojectsettingswidget.cpp clangprojectsettingswidget.h clangprojectsettingswidget.ui
    clangrefactoringengine.cpp clangrefactoringengine.h
    clangtextmark.cpp clangtextmark.h
    clanguiheaderondiskmanager.cpp clanguiheaderondiskmanager.h
    clangutils.cpp clangutils.h
  EXPLICIT_MOC clangcodemodelplugin.h
)

extend_qtc_plugin(ClangCodeModel
  CONDITION WITH_TESTS
  SOURCES
    test/clangautomationutils.cpp test/clangautomationutils.h
    test/clangbatchfileprocessor.cpp test/clangbatchfileprocessor.h
    test/clangcodecompletion_test.cpp test/clangcodecompletion_test.h
    test/data/clangtestdata.qrc
)
