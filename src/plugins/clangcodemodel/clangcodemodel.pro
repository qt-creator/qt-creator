include(../../qtcreatorplugin.pri)
include(../../shared/clang/clang_installation.pri)

LIBS += $$LLVM_LIBS
INCLUDEPATH += $$LLVM_INCLUDEPATH
DEFINES += CLANGCODEMODEL_LIBRARY

# The following defines are used to determine the clang include path for intrinsics
DEFINES += CLANG_VERSION=\\\"$${LLVM_VERSION}\\\"
DEFINES += "\"CLANG_RESOURCE_DIR=\\\"$${LLVM_LIBDIR}/clang/$${LLVM_VERSION}/include\\\"\""

unix:QMAKE_LFLAGS += -Wl,-rpath,\'$$LLVM_LIBDIR\'

SOURCES += \
    activationsequencecontextprocessor.cpp \
    activationsequenceprocessor.cpp \
    clangassistproposal.cpp \
    clangassistproposalitem.cpp \
    clangassistproposalmodel.cpp \
    clangbackendipcintegration.cpp \
    clangcodemodelplugin.cpp \
    clangcompletionassistinterface.cpp \
    clangcompletionassistprocessor.cpp \
    clangcompletionassistprovider.cpp \
    clangcompletioncontextanalyzer.cpp \
    clangeditordocumentparser.cpp \
    clangeditordocumentprocessor.cpp \
    clangfunctionhintmodel.cpp \
    clangmodelmanagersupport.cpp \
    clangprojectsettings.cpp \
    clangprojectsettingspropertiespage.cpp \
    clangutils.cpp \
    completionchunkstotextconverter.cpp \
    cppcreatemarkers.cpp \
    cxprettyprinter.cpp \
    diagnostic.cpp \
    fastindexer.cpp \
    pchinfo.cpp \
    pchmanager.cpp \
    raii/scopedclangoptions.cpp \
    semanticmarker.cpp \
    sourcelocation.cpp \
    sourcemarker.cpp \
    symbol.cpp \
    unit.cpp \
    unsavedfiledata.cpp \
    utils.cpp \
    utils_p.cpp


HEADERS += \
    activationsequencecontextprocessor.h \
    activationsequenceprocessor.h \
    clangassistproposal.h \
    clangassistproposalitem.h \
    clangassistproposalmodel.h \
    clangbackendipcintegration.h \
    clangcodemodelplugin.h \
    clangcompletionassistinterface.h \
    clangcompletionassistprocessor.h \
    clangcompletionassistprovider.h \
    clangcompletioncontextanalyzer.h \
    clangeditordocumentparser.h \
    clangeditordocumentprocessor.h \
    clangfunctionhintmodel.h \
    clang_global.h \
    clangmodelmanagersupport.h \
    clangprojectsettings.h \
    clangprojectsettingspropertiespage.h \
    clangutils.h \
    completionchunkstotextconverter.h \
    constants.h \
    cppcreatemarkers.h \
    cxprettyprinter.h \
    cxraii.h \
    diagnostic.h \
    fastindexer.h \
    pchinfo.h \
    pchmanager.h \
    raii/scopedclangoptions.h \
    semanticmarker.h \
    sourcelocation.h \
    sourcemarker.h \
    symbol.h \
    unit.h \
    unsavedfiledata.h \
    utils.h \
    utils_p.h


contains(DEFINES, CLANG_INDEXING) {
    HEADERS += \
        clangindexer.h \
        index.h \
        indexer.h
#        dependencygraph.h \

    SOURCES += \
        clangindexer.cpp \
        index.cpp \
        indexer.cpp
#        dependencygraph.cpp \
}

FORMS += clangprojectsettingspropertiespage.ui

equals(TEST, 1) {
    RESOURCES += \
        test/clang_tests_database.qrc

    HEADERS += \
        test/clangcodecompletion_test.h

    SOURCES += \
        test/clangcodecompletion_test.cpp

    DISTFILES += \
        test/mysource.cpp \
        test/myheader.cpp \
        test/completionWithProject.cpp \
        test/memberCompletion.cpp \
        test/doxygenKeywordsCompletion.cpp \
        test/preprocessorKeywordsCompletion.cpp \
        test/includeDirectiveCompletion.cpp
}

macx {
    LIBCLANG_VERSION=3.3
    POSTL = install_name_tool -change "@executable_path/../lib/libclang.$${LIBCLANG_VERSION}.dylib" "$$LLVM_INSTALL_DIR/lib/libclang.$${LIBCLANG_VERSION}.dylib" "\"$${DESTDIR}/lib$${TARGET}.dylib\"" $$escape_expand(\\n\\t)
    !isEmpty(QMAKE_POST_LINK):QMAKE_POST_LINK = $$escape_expand(\\n\\t)$$QMAKE_POST_LINK
    QMAKE_POST_LINK = $$POSTL $$QMAKE_POST_LINK
}
