include(../../qtcreatorplugin.pri)
include(clang_installation.pri)

message("Building ClangCodeModel plugin with Clang from $$LLVM_INSTALL_DIR")
message("  INCLUDEPATH += $$LLVM_INCLUDEPATH")
message("  LIBS += $$LLVM_LIBS")

LIBS += $$LLVM_LIBS
INCLUDEPATH += $$LLVM_INCLUDEPATH
DEFINES += CLANGCODEMODEL_LIBRARY

unix:QMAKE_LFLAGS += -Wl,-rpath,\'$$LLVM_LIBDIR\'

contains(DEFINES, CLANG_COMPLETION) {
    HEADERS += clangcompletion.h clangcompleter.h completionproposalsbuilder.h
    SOURCES += clangcompletion.cpp clangcompleter.cpp completionproposalsbuilder.cpp
}

contains(DEFINES, CLANG_HIGHLIGHTING) {
    HEADERS += cppcreatemarkers.h
    SOURCES += cppcreatemarkers.cpp
}

SOURCES += \
    $$PWD/clangcodemodelplugin.cpp \
    $$PWD/clangeditordocumentparser.cpp \
    $$PWD/clangeditordocumentprocessor.cpp \
    $$PWD/sourcemarker.cpp \
    $$PWD/symbol.cpp \
    $$PWD/sourcelocation.cpp \
    $$PWD/unit.cpp \
    $$PWD/utils.cpp \
    $$PWD/utils_p.cpp \
    $$PWD/semanticmarker.cpp \
    $$PWD/diagnostic.cpp \
    $$PWD/unsavedfiledata.cpp \
    $$PWD/fastindexer.cpp \
    $$PWD/pchinfo.cpp \
    $$PWD/pchmanager.cpp \
    $$PWD/clangprojectsettings.cpp \
    $$PWD/clangprojectsettingspropertiespage.cpp \
    $$PWD/raii/scopedclangoptions.cpp \
    $$PWD/clangmodelmanagersupport.cpp

HEADERS += \
    $$PWD/clangcodemodelplugin.h \
    $$PWD/clangeditordocumentparser.h \
    $$PWD/clangeditordocumentprocessor.h \
    $$PWD/clang_global.h \
    $$PWD/sourcemarker.h \
    $$PWD/constants.h \
    $$PWD/symbol.h \
    $$PWD/cxraii.h \
    $$PWD/sourcelocation.h \
    $$PWD/unit.h \
    $$PWD/utils.h \
    $$PWD/utils_p.h \
    $$PWD/semanticmarker.h \
    $$PWD/diagnostic.h \
    $$PWD/unsavedfiledata.h \
    $$PWD/fastindexer.h \
    $$PWD/pchinfo.h \
    $$PWD/pchmanager.h \
    $$PWD/clangprojectsettings.h \
    $$PWD/clangprojectsettingspropertiespage.h \
    $$PWD/raii/scopedclangoptions.h \
    $$PWD/clangmodelmanagersupport.h

HEADERS += clangutils.h \
    cxprettyprinter.h

SOURCES += clangutils.cpp \
    cxprettyprinter.cpp

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

equals(TEST, 1) {
    RESOURCES += \
        $$PWD/test/clang_tests_database.qrc

    HEADERS += \
        $$PWD/test/completiontesthelper.h

    SOURCES += \
        $$PWD/test/completiontesthelper.cpp \
        $$PWD/test/clangcompletion_test.cpp

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
        test/cxx_snippets_4.cpp \
        test/objc_messages_1.mm \
        test/objc_messages_2.mm \
        test/objc_messages_3.mm
}

FORMS += $$PWD/clangprojectsettingspropertiespage.ui

macx {
    LIBCLANG_VERSION=3.3
    POSTL = install_name_tool -change "@executable_path/../lib/libclang.$${LIBCLANG_VERSION}.dylib" "$$LLVM_INSTALL_DIR/lib/libclang.$${LIBCLANG_VERSION}.dylib" "\"$${DESTDIR}/lib$${TARGET}.dylib\"" $$escape_expand(\\n\\t)
    !isEmpty(QMAKE_POST_LINK):QMAKE_POST_LINK = $$escape_expand(\\n\\t)$$QMAKE_POST_LINK
    QMAKE_POST_LINK = $$POSTL $$QMAKE_POST_LINK
}
