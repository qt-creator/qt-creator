DEFINES += CPPTOOLS_LIBRARY
win32-msvc*:DEFINES += _SCL_SECURE_NO_WARNINGS
include(../../qtcreatorplugin.pri)

HEADERS += \
    abstracteditorsupport.h \
    baseeditordocumentparser.h \
    baseeditordocumentprocessor.h \
    builtineditordocumentparser.h \
    builtineditordocumentprocessor.h \
    builtinindexingsupport.h \
    commentssettings.h \
    completionsettingspage.h \
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
    cppcurrentdocumentfilter.h \
    cppeditoroutline.h \
    cppdoxygen.h \
    cppfilesettingspage.h \
    cppfindreferences.h \
    cppfunctionsfilter.h \
    cppincludesfilter.h \
    cppindexingsupport.h \
    cpplocalsymbols.h \
    cpplocatordata.h \
    cpplocatorfilter.h \
    cppmodelmanager.h \
    cppmodelmanagersupport.h \
    cppmodelmanagersupportinternal.h \
    cpppointerdeclarationformatter.h \
    cppprojectfile.h \
    cppprojects.h \
    cppqtstyleindenter.h \
    cpprefactoringchanges.h \
    cppsemanticinfo.h \
    cppsemanticinfoupdater.h \
    cppsourceprocessor.h \
    cpptools_global.h \
    cpptoolsconstants.h \
    cpptoolsjsextension.h \
    cpptoolsplugin.h \
    cpptoolsreuse.h \
    cpptoolssettings.h \
    cppworkingcopy.h \
    doxygengenerator.h \
    editordocumenthandle.h \
    functionutils.h \
    includeutils.h \
    indexitem.h \
    insertionpointlocator.h \
    searchsymbols.h \
    semantichighlighter.h \
    stringtable.h \
    symbolfinder.h \
    symbolsfindfilter.h \
    typehierarchybuilder.h

SOURCES += \
    abstracteditorsupport.cpp \
    baseeditordocumentparser.cpp \
    baseeditordocumentprocessor.cpp \
    builtineditordocumentparser.cpp \
    builtineditordocumentprocessor.cpp \
    builtinindexingsupport.cpp \
    commentssettings.cpp \
    completionsettingspage.cpp \
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
    cppfilesettingspage.cpp \
    cppfindreferences.cpp \
    cppfunctionsfilter.cpp \
    cppincludesfilter.cpp \
    cppindexingsupport.cpp \
    cpplocalsymbols.cpp \
    cpplocatordata.cpp \
    cpplocatorfilter.cpp \
    cppmodelmanager.cpp \
    cppmodelmanagersupport.cpp \
    cppmodelmanagersupportinternal.cpp \
    cpppointerdeclarationformatter.cpp \
    cppprojectfile.cpp \
    cppprojects.cpp \
    cppqtstyleindenter.cpp \
    cpprefactoringchanges.cpp \
    cppsemanticinfo.cpp \
    cppsemanticinfoupdater.cpp \
    cppsourceprocessor.cpp \
    cpptoolsjsextension.cpp \
    cpptoolsplugin.cpp \
    cpptoolsreuse.cpp \
    cpptoolssettings.cpp \
    cppworkingcopy.cpp \
    doxygengenerator.cpp \
    editordocumenthandle.cpp \
    functionutils.cpp \
    includeutils.cpp \
    indexitem.cpp \
    insertionpointlocator.cpp \
    searchsymbols.cpp \
    semantichighlighter.cpp \
    stringtable.cpp \
    symbolfinder.cpp \
    symbolsfindfilter.cpp \
    typehierarchybuilder.cpp

FORMS += \
    completionsettingspage.ui \
    cppcodemodelsettingspage.ui \
    cppcodestylesettingspage.ui \
    cppfilesettingspage.ui

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
