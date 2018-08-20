include(../../qtcreatorplugin.pri)
include(../../shared/clang/clang_installation.pri)

include(../../shared/clang/clang_defines.pri)

requires(!isEmpty(LLVM_VERSION))

SOURCES += \
    clangactivationsequencecontextprocessor.cpp \
    clangactivationsequenceprocessor.cpp \
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
    clangcurrentdocumentfilter.cpp \
    clangdiagnosticfilter.cpp \
    clangdiagnosticmanager.cpp \
    clangdiagnostictooltipwidget.cpp \
    clangeditordocumentparser.cpp \
    clangeditordocumentprocessor.cpp \
    clangfixitoperation.cpp \
    clangfixitoperationsextractor.cpp \
    clangfollowsymbol.cpp \
    clangfunctionhintmodel.cpp \
    clanghighlightingresultreporter.cpp \
    clanghoverhandler.cpp \
    clangmodelmanagersupport.cpp \
    clangpreprocessorassistproposalitem.cpp \
    clangprojectsettings.cpp \
    clangprojectsettingswidget.cpp \
    clangrefactoringengine.cpp \
    clangtextmark.cpp \
    clanguiheaderondiskmanager.cpp \
    clangutils.cpp \
    clangoverviewmodel.cpp

HEADERS += \
    clangactivationsequencecontextprocessor.h \
    clangactivationsequenceprocessor.h \
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
    clangcurrentdocumentfilter.h \
    clangdiagnosticfilter.h \
    clangdiagnosticmanager.h \
    clangdiagnostictooltipwidget.h \
    clangeditordocumentparser.h \
    clangeditordocumentprocessor.h \
    clangfixitoperation.h \
    clangfixitoperationsextractor.h \
    clangfollowsymbol.h \
    clangfunctionhintmodel.h \
    clanghighlightingresultreporter.h \
    clanghoverhandler.h \
    clangisdiagnosticrelatedtolocation.h \
    clangmodelmanagersupport.h \
    clangpreprocessorassistproposalitem.h \
    clangprojectsettings.h \
    clangprojectsettingswidget.h \
    clangrefactoringengine.h \
    clangtextmark.h \
    clanguiheaderondiskmanager.h \
    clangutils.h \
    clangoverviewmodel.h

FORMS += clangprojectsettingswidget.ui

DISTFILES += \
    README \
    $${IDE_SOURCE_TREE}/doc/src/editors/creator-only/creator-clang-codemodel.qdoc

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
