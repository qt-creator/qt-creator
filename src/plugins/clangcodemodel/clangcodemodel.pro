include(../../qtcreatorplugin.pri)
include(../../shared/clang/clang_installation.pri)

include(../../shared/clang/clang_defines.pri)

SOURCES += \
    clangactivationsequencecontextprocessor.cpp \
    clangactivationsequenceprocessor.cpp \
    clangassistproposal.cpp \
    clangassistproposalitem.cpp \
    clangassistproposalmodel.cpp \
    clangbackendcommunicator.cpp \
    clangbackendlogging.cpp \
    clangbackendreceiver.cpp \
    clangbackendsender.cpp \
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
    clangfollowsymbol.cpp \
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
    clangbackendcommunicator.h \
    clangbackendlogging.h \
    clangbackendreceiver.h \
    clangbackendsender.h \
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
    clangfollowsymbol.h \
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
        test/clangautomationutils.h \
        test/clangbatchfileprocessor.h \
        test/clangcodecompletion_test.h \

    SOURCES += \
        test/clangautomationutils.cpp \
        test/clangbatchfileprocessor.cpp \
        test/clangcodecompletion_test.cpp \

    RESOURCES += test/data/clangtestdata.qrc
    OTHER_FILES += $$files(test/data/*)
}
