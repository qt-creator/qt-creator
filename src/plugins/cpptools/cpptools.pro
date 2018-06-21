DEFINES += CPPTOOLS_LIBRARY
msvc: DEFINES += _SCL_SECURE_NO_WARNINGS
include(../../qtcreatorplugin.pri)

HEADERS += \
    abstracteditorsupport.h \
    abstractoverviewmodel.h \
    baseeditordocumentparser.h \
    baseeditordocumentprocessor.h \
    builtincursorinfo.h \
    builtineditordocumentparser.h \
    builtineditordocumentprocessor.h \
    builtinindexingsupport.h \
    clangdiagnosticconfig.h \
    clangdiagnosticconfigsmodel.h \
    clangdiagnosticconfigsselectionwidget.h \
    clangdiagnosticconfigswidget.h \
    cppcanonicalsymbol.h \
    cppchecksymbols.h \
    cppclassesfilter.h \
    cppcodeformatter.h \
    cppcodemodelinspectordumper.h \
    cppcodemodelsettings.h \
    cppcodemodelsettingspage.h \
    cppcodestylepreferences.h \
    cppcodestylepreferencesfactory.h \
    cppcodestylesettings.h \
    cppcodestylesettingspage.h \
    cppcompletionassist.h \
    cppcompletionassistprocessor.h \
    cppcompletionassistprovider.h \
    cppcursorinfo.h \
    cppcurrentdocumentfilter.h \
    cppdoxygen.h \
    cppeditoroutline.h \
    cppeditorwidgetinterface.h \
    cppelementevaluator.h \
    cppfileiterationorder.h \
    cppfilesettingspage.h \
    cppfindreferences.h \
    cppfollowsymbolundercursor.h \
    cppfunctionsfilter.h \
    cpphoverhandler.h \
    cppincludesfilter.h \
    cppindexingsupport.h \
    cpplocalsymbols.h \
    cpplocatordata.h \
    cpplocatorfilter.h \
    cppmodelmanager.h \
    cppmodelmanagersupport.h \
    cppmodelmanagersupportinternal.h \
    cppoverviewmodel.h \
    cpppointerdeclarationformatter.h \
    cppprojectfile.h \
    cppprojectupdater.h \
    cppqtstyleindenter.h \
    cpprawprojectpart.h \
    cpprefactoringchanges.h \
    cpprefactoringengine.h \
    cppselectionchanger.h \
    cppsemanticinfo.h \
    cppsemanticinfoupdater.h \
    cppsourceprocessor.h \
    cpptools_global.h \
    cpptools_utils.h \
    cpptoolsconstants.h \
    cpptoolsjsextension.h \
    cpptoolsplugin.h \
    cpptoolsreuse.h \
    cpptoolssettings.h \
    cppvirtualfunctionassistprovider.h \
    cppvirtualfunctionproposalitem.h \
    cppworkingcopy.h \
    doxygengenerator.h \
    editordocumenthandle.h \
    followsymbolinterface.h \
    functionutils.h \
    generatedcodemodelsupport.h \
    includeutils.h \
    indexitem.h \
    insertionpointlocator.h \
    searchsymbols.h \
    semantichighlighter.h \
    stringtable.h \
    symbolfinder.h \
    symbolsfindfilter.h \
    typehierarchybuilder.h \
    senddocumenttracker.h \
    cpptoolsbridge.h \
    cpptoolsbridgeinterface.h \
    cpptoolsbridgeqtcreatorimplementation.h \
    projectpart.h \
    projectpartheaderpath.h \
    projectinfo.h \
    cppprojectinfogenerator.h \
    compileroptionsbuilder.h \
    refactoringengineinterface.h \
    cppprojectfilecategorizer.h \
    cppprojectpartchooser.h \
    cppsymbolinfo.h \
    cursorineditor.h \
    wrappablelineedit.h \
    usages.h \
    cpptools_clangtidychecks.h

SOURCES += \
    abstracteditorsupport.cpp \
    baseeditordocumentparser.cpp \
    baseeditordocumentprocessor.cpp \
    builtincursorinfo.cpp \
    builtineditordocumentparser.cpp \
    builtineditordocumentprocessor.cpp \
    builtinindexingsupport.cpp \
    clangdiagnosticconfig.cpp \
    clangdiagnosticconfigsmodel.cpp \
    clangdiagnosticconfigsselectionwidget.cpp \
    clangdiagnosticconfigswidget.cpp \
    cppcanonicalsymbol.cpp \
    cppchecksymbols.cpp \
    cppclassesfilter.cpp \
    cppcodeformatter.cpp \
    cppcodemodelinspectordumper.cpp \
    cppcodemodelsettings.cpp \
    cppcodemodelsettingspage.cpp \
    cppcodestylepreferences.cpp \
    cppcodestylepreferencesfactory.cpp \
    cppcodestylesettings.cpp \
    cppcodestylesettingspage.cpp \
    cppcompletionassist.cpp \
    cppcompletionassistprocessor.cpp \
    cppcompletionassistprovider.cpp \
    cppcurrentdocumentfilter.cpp \
    cppeditoroutline.cpp \
    cppdoxygen.cpp \
    cppelementevaluator.cpp \
    cppfileiterationorder.cpp \
    cppfilesettingspage.cpp \
    cppfindreferences.cpp \
    cppfollowsymbolundercursor.cpp \
    cppfunctionsfilter.cpp \
    cpphoverhandler.cpp \
    cppincludesfilter.cpp \
    cppindexingsupport.cpp \
    cpplocalsymbols.cpp \
    cpplocatordata.cpp \
    cpplocatorfilter.cpp \
    cppmodelmanager.cpp \
    cppmodelmanagersupport.cpp \
    cppmodelmanagersupportinternal.cpp \
    cppoverviewmodel.cpp \
    cpppointerdeclarationformatter.cpp \
    cppprojectfile.cpp \
    cppprojectupdater.cpp \
    cppqtstyleindenter.cpp \
    cpprawprojectpart.cpp \
    cpprefactoringchanges.cpp \
    cpprefactoringengine.cpp \
    cppselectionchanger.cpp \
    cppsemanticinfoupdater.cpp \
    cppsourceprocessor.cpp \
    cpptoolsjsextension.cpp \
    cpptoolsplugin.cpp \
    cpptoolsreuse.cpp \
    cpptoolssettings.cpp \
    cppvirtualfunctionassistprovider.cpp \
    cppvirtualfunctionproposalitem.cpp \
    cppworkingcopy.cpp \
    doxygengenerator.cpp \
    editordocumenthandle.cpp \
    functionutils.cpp \
    generatedcodemodelsupport.cpp \
    includeutils.cpp \
    indexitem.cpp \
    insertionpointlocator.cpp \
    searchsymbols.cpp \
    semantichighlighter.cpp \
    stringtable.cpp \
    symbolfinder.cpp \
    symbolsfindfilter.cpp \
    typehierarchybuilder.cpp \
    senddocumenttracker.cpp \
    cpptoolsbridge.cpp \
    cpptoolsbridgeqtcreatorimplementation.cpp \
    projectpart.cpp \
    projectinfo.cpp \
    cppprojectinfogenerator.cpp \
    compileroptionsbuilder.cpp \
    cppprojectfilecategorizer.cpp \
    cppprojectpartchooser.cpp \
    wrappablelineedit.cpp

FORMS += \
    clangdiagnosticconfigswidget.ui \
    cppcodemodelsettingspage.ui \
    cppcodestylesettingspage.ui \
    cppfilesettingspage.ui \
    clangbasechecks.ui \
    clazychecks.ui \
    tidychecks.ui

equals(TEST, 1) {
    HEADERS += \
        cppsourceprocessertesthelper.h \
        cpptoolstestcase.h \
        modelmanagertesthelper.h

    SOURCES += \
        cppcodegen_test.cpp \
        cppcompletion_test.cpp \
        cppheadersource_test.cpp \
        cpplocalsymbols_test.cpp \
        cpplocatorfilter_test.cpp \
        cppmodelmanager_test.cpp \
        cpppointerdeclarationformatter_test.cpp \
        cppsourceprocessertesthelper.cpp \
        cppsourceprocessor_test.cpp \
        cpptoolstestcase.cpp \
        modelmanagertesthelper.cpp \
        symbolsearcher_test.cpp \
        typehierarchybuilder_test.cpp

    DEFINES += SRCDIR=\\\"$$PWD\\\"
}

RESOURCES += \
    cpptools.qrc
