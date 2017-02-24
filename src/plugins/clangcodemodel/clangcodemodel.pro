include(../../qtcreatorplugin.pri)
include(../../shared/clang/clang_installation.pri)

include(../../shared/clang/clang_defines.pri)

SOURCES += \
    clangactivationsequencecontextprocessor.cpp \
    clangactivationsequenceprocessor.cpp \
    clangassistproposal.cpp \
    clangassistproposalitem.cpp \
    clangassistproposalmodel.cpp \
    clangbackendipcintegration.cpp \
    clangcodemodelplugin.cpp \
    clangcompletionassistinterface.cpp \
    clangcompletionassistprocessor.cpp \
    clangcompletionassistprovider.cpp \
    clangcompletionchunkstotextconverter.cpp \
    clangcompletioncontextanalyzer.cpp \
    clangdiagnosticfilter.cpp \
    clangdiagnosticmanager.cpp \
    clangdiagnostictooltipwidget.cpp \
    clangeditordocumentparser.cpp \
    clangeditordocumentprocessor.cpp \
    clangfixitoperation.cpp \
    clangfixitoperationsextractor.cpp \
    clangfunctionhintmodel.cpp \
    clanghighlightingmarksreporter.cpp \
    clangmodelmanagersupport.cpp \
    clangpreprocessorassistproposalitem.cpp \
    clangprojectsettings.cpp \
    clangprojectsettingswidget.cpp \
    clangtextmark.cpp \
    clanguiheaderondiskmanager.cpp \
    clangutils.cpp

HEADERS += \
    clangactivationsequencecontextprocessor.h \
    clangactivationsequenceprocessor.h \
    clangassistproposal.h \
    clangassistproposalitem.h \
    clangassistproposalmodel.h \
    clangbackendipcintegration.h \
    clangcodemodelplugin.h \
    clangcompletionassistinterface.h \
    clangcompletionassistprocessor.h \
    clangcompletionassistprovider.h \
    clangcompletionchunkstotextconverter.h \
    clangcompletioncontextanalyzer.h \
    clangconstants.h \
    clangdiagnosticfilter.h \
    clangdiagnosticmanager.h \
    clangdiagnostictooltipwidget.h \
    clangeditordocumentparser.h \
    clangeditordocumentprocessor.h \
    clangfixitoperation.h \
    clangfixitoperationsextractor.h \
    clangfunctionhintmodel.h \
    clanghighlightingmarksreporter.h \
    clangisdiagnosticrelatedtolocation.h \
    clangmodelmanagersupport.h \
    clangpreprocessorassistproposalitem.h \
    clangprojectsettings.h \
    clangprojectsettingswidget.h \
    clangtextmark.h \
    clanguiheaderondiskmanager.h \
    clangutils.h

FORMS += clangprojectsettingswidget.ui

RESOURCES += \
    clangcodemodel.qrc

DISTFILES += \
    README \
    $${IDE_SOURCE_TREE}/doc/src/editors/creator-clang-codemodel.qdoc

equals(TEST, 1) {
    HEADERS += \
        test/clangcodecompletion_test.h

    SOURCES += \
        test/clangcodecompletion_test.cpp

    RESOURCES += test/data/clangtestdata.qrc
    OTHER_FILES += $$files(test/data/*)
}
