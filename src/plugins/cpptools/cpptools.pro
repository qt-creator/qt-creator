include(../../qtcreatorplugin.pri)

DEFINES += CPPTOOLS_LIBRARY
HEADERS += completionsettingspage.h \
    cppclassesfilter.h \
    cppcurrentdocumentfilter.h \
    cppfunctionsfilter.h \
    cppmodelmanager.h \
    cppmodelmanagerinterface.h \
    cpplocatorfilter.h \
    cpptools_global.h \
    cpptoolsconstants.h \
    cpptoolseditorsupport.h \
    cpptoolsplugin.h \
    cppqtstyleindenter.h \
    searchsymbols.h \
    cppdoxygen.h \
    cppfilesettingspage.h \
    cppfindreferences.h \
    cppcodeformatter.h \
    symbolsfindfilter.h \
    uicodecompletionsupport.h \
    insertionpointlocator.h \
    cpprefactoringchanges.h \
    abstracteditorsupport.h \
    cppcompletionassist.h \
    cppcodestylesettingspage.h \
    cpptoolssettings.h \
    cppcodestylesettings.h \
    cppcodestylepreferencesfactory.h \
    cppcodestylepreferences.h \
    cpptoolsreuse.h \
    doxygengenerator.h \
    commentssettings.h \
    symbolfinder.h \
    cppcompletionsupport.h \
    cpphighlightingsupport.h \
    cpphighlightingsupportinternal.h \
    cppchecksymbols.h \
    cpplocalsymbols.h \
    cppsemanticinfo.h \
    cppcompletionassistprovider.h \
    typehierarchybuilder.h \
    cppindexingsupport.h \
    builtinindexingsupport.h \
    cpppointerdeclarationformatter.h \
    cppprojectfile.h \
    cpppreprocessor.h

SOURCES += completionsettingspage.cpp \
    cppclassesfilter.cpp \
    cppcurrentdocumentfilter.cpp \
    cppfunctionsfilter.cpp \
    cppmodelmanager.cpp \
    cppmodelmanagerinterface.cpp \
    cpplocatorfilter.cpp \
    cpptoolseditorsupport.cpp \
    cpptoolsplugin.cpp \
    cppqtstyleindenter.cpp \
    searchsymbols.cpp \
    cppdoxygen.cpp \
    cppfilesettingspage.cpp \
    abstracteditorsupport.cpp \
    cppfindreferences.cpp \
    cppcodeformatter.cpp \
    symbolsfindfilter.cpp \
    uicodecompletionsupport.cpp \
    insertionpointlocator.cpp \
    cpprefactoringchanges.cpp \
    cppcompletionassist.cpp \
    cppcodestylesettingspage.cpp \
    cpptoolssettings.cpp \
    cppcodestylesettings.cpp \
    cppcodestylepreferencesfactory.cpp \
    cppcodestylepreferences.cpp \
    cpptoolsreuse.cpp \
    doxygengenerator.cpp \
    commentssettings.cpp \
    symbolfinder.cpp \
    cppcompletionsupport.cpp \
    cpphighlightingsupport.cpp \
    cpphighlightingsupportinternal.cpp \
    cppchecksymbols.cpp \
    cpplocalsymbols.cpp \
    cppsemanticinfo.cpp \
    cppcompletionassistprovider.cpp \
    typehierarchybuilder.cpp \
    cppindexingsupport.cpp \
    builtinindexingsupport.cpp \
    cpppointerdeclarationformatter.cpp \
    cppprojectfile.cpp \
    cpppreprocessor.cpp

FORMS += completionsettingspage.ui \
    cppfilesettingspage.ui \
    cppcodestylesettingspage.ui

equals(TEST, 1) {
    SOURCES += \
        cppcodegen_test.cpp \
        cppcompletion_test.cpp \
        cppmodelmanager_test.cpp \
        modelmanagertesthelper.cpp \
        cpppointerdeclarationformatter_test.cpp

    HEADERS += \
        modelmanagertesthelper.h

    DEFINES += SRCDIR=\\\"$$PWD\\\"
}
