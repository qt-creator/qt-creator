add_qtc_library(QmlJS
  DEPENDS ExtensionSystem Utils
  PUBLIC_DEPENDS CPlusPlus Qt5::Widgets Qt5::Xml LanguageUtils
  SOURCES
    jsoncheck.cpp jsoncheck.h
    parser/qmldirparser.cpp parser/qmldirparser_p.h
    parser/qmljsast.cpp parser/qmljsast_p.h
    parser/qmljsastfwd_p.h
    parser/qmljsastvisitor.cpp parser/qmljsastvisitor_p.h
    parser/qmljsdiagnosticmessage_p.h
    parser/qmljsengine_p.cpp parser/qmljsengine_p.h
    parser/qmljsglobal_p.h
    parser/qmljsgrammar.cpp parser/qmljsgrammar_p.h
    parser/qmljskeywords_p.h
    parser/qmljslexer.cpp parser/qmljslexer_p.h
    parser/qmljsmemorypool_p.h
    parser/qmljsparser.cpp parser/qmljsparser_p.h
    parser/qmljssourcelocation_p.h
    persistenttrie.cpp persistenttrie.h
    qmljs_global.h
    qmljsbind.cpp qmljsbind.h
    qmljsbundle.cpp qmljsbundle.h
    qmljscheck.cpp qmljscheck.h
    qmljscodeformatter.cpp qmljscodeformatter.h
    qmljscompletioncontextfinder.cpp qmljscompletioncontextfinder.h
    qmljsconstants.h
    qmljscontext.cpp qmljscontext.h
    qmljsdialect.cpp qmljsdialect.h
    qmljsdocument.cpp qmljsdocument.h
    qmljsevaluate.cpp qmljsevaluate.h
    qmljsfindexportedcpptypes.cpp qmljsfindexportedcpptypes.h
    qmljsicons.cpp qmljsicons.h
    qmljsicontextpane.h
    qmljsimportdependencies.cpp qmljsimportdependencies.h
    qmljsindenter.cpp qmljsindenter.h
    qmljsinterpreter.cpp qmljsinterpreter.h
    qmljslineinfo.cpp qmljslineinfo.h
    qmljslink.cpp qmljslink.h
    qmljsmodelmanagerinterface.cpp qmljsmodelmanagerinterface.h
    qmljsplugindumper.cpp qmljsplugindumper.h
    qmljspropertyreader.cpp qmljspropertyreader.h
    qmljsreformatter.cpp qmljsreformatter.h
    qmljsrewriter.cpp qmljsrewriter.h
    qmljsscanner.cpp qmljsscanner.h
    qmljsscopeastpath.cpp qmljsscopeastpath.h
    qmljsscopebuilder.cpp qmljsscopebuilder.h
    qmljsscopechain.cpp qmljsscopechain.h
    qmljssimplereader.cpp qmljssimplereader.h
    qmljsstaticanalysismessage.cpp qmljsstaticanalysismessage.h
    qmljstypedescriptionreader.cpp qmljstypedescriptionreader.h
    qmljsutils.cpp qmljsutils.h
    qmljsvalueowner.cpp qmljsvalueowner.h
    qmljsviewercontext.h
)
