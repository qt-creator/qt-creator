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
    $$PWD/clangcodemodelplugin.cpp \
    $$PWD/clangcompleter.cpp \
    $$PWD/clangcompletion.cpp \
    $$PWD/clangeditordocumentparser.cpp \
    $$PWD/clangeditordocumentprocessor.cpp \
    $$PWD/clangmodelmanagersupport.cpp \
    $$PWD/clangprojectsettings.cpp \
    $$PWD/clangprojectsettingspropertiespage.cpp \
    $$PWD/clangutils.cpp \
    $$PWD/completionproposalsbuilder.cpp \
    $$PWD/cppcreatemarkers.cpp \
    $$PWD/cxprettyprinter.cpp \
    $$PWD/diagnostic.cpp \
    $$PWD/fastindexer.cpp \
    $$PWD/pchinfo.cpp \
    $$PWD/pchmanager.cpp \
    $$PWD/raii/scopedclangoptions.cpp \
    $$PWD/semanticmarker.cpp \
    $$PWD/sourcelocation.cpp \
    $$PWD/sourcemarker.cpp \
    $$PWD/symbol.cpp \
    $$PWD/unit.cpp \
    $$PWD/unsavedfiledata.cpp \
    $$PWD/utils.cpp \
    $$PWD/utils_p.cpp

HEADERS += \
    $$PWD/clangcodemodelplugin.h \
    $$PWD/clangcompleter.h \
    $$PWD/clangcompletion.h \
    $$PWD/clangeditordocumentparser.h \
    $$PWD/clangeditordocumentprocessor.h \
    $$PWD/clang_global.h \
    $$PWD/clangmodelmanagersupport.h \
    $$PWD/clangprojectsettings.h \
    $$PWD/clangprojectsettingspropertiespage.h \
    $$PWD/clangutils.h \
    $$PWD/completionproposalsbuilder.h \
    $$PWD/constants.h \
    $$PWD/cppcreatemarkers.h \
    $$PWD/cxprettyprinter.h \
    $$PWD/cxraii.h \
    $$PWD/diagnostic.h \
    $$PWD/fastindexer.h \
    $$PWD/pchinfo.h \
    $$PWD/pchmanager.h \
    $$PWD/raii/scopedclangoptions.h \
    $$PWD/semanticmarker.h \
    $$PWD/sourcelocation.h \
    $$PWD/sourcemarker.h \
    $$PWD/symbol.h \
    $$PWD/unit.h \
    $$PWD/unsavedfiledata.h \
    $$PWD/utils.h \
    $$PWD/utils_p.h

contains(DEFINES, CLANG_INDEXING) {
    HEADERS += \
        $$PWD/clangindexer.h \
        $$PWD/index.h \
        $$PWD/indexer.h
#        $$PWD/dependencygraph.h \

    SOURCES += \
        $$PWD/clangindexer.cpp \
        $$PWD/index.cpp \
        $$PWD/indexer.cpp
#        $$PWD/dependencygraph.cpp \
}

FORMS += $$PWD/clangprojectsettingspropertiespage.ui

equals(TEST, 1) {
    RESOURCES += \
        $$PWD/test/clang_tests_database.qrc

    HEADERS += \
        $$PWD/test/completiontesthelper.h

    SOURCES += \
        $$PWD/test/clangcompletion_test.cpp \
        $$PWD/test/completiontesthelper.cpp

    DISTFILES += \
        $$PWD/test/cxx_regression_1.cpp \
        $$PWD/test/cxx_regression_2.cpp \
        $$PWD/test/cxx_regression_3.cpp \
        $$PWD/test/cxx_regression_4.cpp \
        $$PWD/test/cxx_regression_5.cpp \
        $$PWD/test/cxx_regression_6.cpp \
        $$PWD/test/cxx_regression_7.cpp \
        $$PWD/test/cxx_regression_8.cpp \
        $$PWD/test/cxx_regression_9.cpp \
        $$PWD/test/cxx_snippets_1.cpp \
        $$PWD/test/cxx_snippets_2.cpp \
        $$PWD/test/cxx_snippets_3.cpp \
        $$PWD/test/cxx_snippets_4.cpp \
        $$PWD/test/objc_messages_1.mm \
        $$PWD/test/objc_messages_2.mm \
        $$PWD/test/objc_messages_3.mm
}

macx {
    LIBCLANG_VERSION=3.3
    POSTL = install_name_tool -change "@executable_path/../lib/libclang.$${LIBCLANG_VERSION}.dylib" "$$LLVM_INSTALL_DIR/lib/libclang.$${LIBCLANG_VERSION}.dylib" "\"$${DESTDIR}/lib$${TARGET}.dylib\"" $$escape_expand(\\n\\t)
    !isEmpty(QMAKE_POST_LINK):QMAKE_POST_LINK = $$escape_expand(\\n\\t)$$QMAKE_POST_LINK
    QMAKE_POST_LINK = $$POSTL $$QMAKE_POST_LINK
}
