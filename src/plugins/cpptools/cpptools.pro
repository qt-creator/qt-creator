include(../../qtcreatorplugin.pri)

DEFINES += CPPTOOLS_LIBRARY
win32-msvc*:DEFINES += _SCL_SECURE_NO_WARNINGS
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
    cppsnapshotupdater.h \
    cpptoolsplugin.h \
    cppqtstyleindenter.h \
    searchsymbols.h \
    cppdoxygen.h \
    cppfilesettingspage.h \
    cppfindreferences.h \
    cppcodeformatter.h \
    symbolsfindfilter.h \
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
    cppmodelmanagersupport.h \
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
    cpppreprocessor.h \
    includeutils.h \
    cpplocatordata.h \
    cppmodelmanagersupportinternal.h \
    cppcodemodelsettings.h \
    cppcodemodelsettingspage.h

SOURCES += completionsettingspage.cpp \
    cppclassesfilter.cpp \
    cppcurrentdocumentfilter.cpp \
    cppfunctionsfilter.cpp \
    cppmodelmanager.cpp \
    cppmodelmanagerinterface.cpp \
    cpplocatorfilter.cpp \
    cpptoolseditorsupport.cpp \
    cppsnapshotupdater.cpp \
    cpptoolsplugin.cpp \
    cppqtstyleindenter.cpp \
    searchsymbols.cpp \
    cppdoxygen.cpp \
    cppfilesettingspage.cpp \
    abstracteditorsupport.cpp \
    cppfindreferences.cpp \
    cppcodeformatter.cpp \
    symbolsfindfilter.cpp \
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
    cppmodelmanagersupport.cpp \
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
    cpppreprocessor.cpp \
    includeutils.cpp \
    cpplocatordata.cpp \
    cppmodelmanagersupportinternal.cpp \
    cppcodemodelsettings.cpp \
    cppcodemodelsettingspage.cpp

FORMS += completionsettingspage.ui \
    cppfilesettingspage.ui \
    cppcodestylesettingspage.ui \
    cppcodemodelsettingspage.ui

equals(TEST, 1) {
    SOURCES += \
        cppcodegen_test.cpp \
        cppcompletion_test.cpp \
        cppmodelmanager_test.cpp \
        modelmanagertesthelper.cpp \
        cpppointerdeclarationformatter_test.cpp \
        cpplocatorfilter_test.cpp \
        symbolsearcher_test.cpp \
        cpppreprocessor_test.cpp \
        cpppreprocessertesthelper.cpp \
        cppheadersource_test.cpp \
        typehierarchybuilder_test.cpp

    HEADERS += \
        cpppreprocessertesthelper.h \
        modelmanagertesthelper.h

    DEFINES += SRCDIR=\\\"$$PWD\\\"
}
