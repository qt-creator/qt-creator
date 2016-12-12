import qbs 1.0

Project {
    name: "QmlJS"

    QtcDevHeaders { }

    QtcLibrary {
        cpp.defines: base.concat(["QMLJS_LIBRARY"])
        cpp.optimization: "fast"

        Depends { name: "ExtensionSystem" }
        Depends { name: "Utils" }
        Depends { name: "LanguageUtils" }
        Depends { name: "CPlusPlus" }
        Depends { name: "Qt"; submodules: ["widgets", "xml"] }

        Group {
            name: "General"
            files: [
                "jsoncheck.cpp", "jsoncheck.h",
                "persistenttrie.cpp", "persistenttrie.h",
                "qmljs_global.h",
                "qmljsbind.cpp", "qmljsbind.h",
                "qmljsbundle.cpp", "qmljsbundle.h",
                "qmljscheck.cpp", "qmljscheck.h",
                "qmljscodeformatter.cpp", "qmljscodeformatter.h",
                "qmljscompletioncontextfinder.cpp", "qmljscompletioncontextfinder.h",
                "qmljsconstants.h",
                "qmljscontext.cpp", "qmljscontext.h",
                "qmljsdescribevalue.cpp", "qmljsdescribevalue.h",
                "qmljsdocument.cpp", "qmljsdocument.h",
                "qmljsevaluate.cpp", "qmljsevaluate.h",
                "qmljsfindexportedcpptypes.cpp", "qmljsfindexportedcpptypes.h",
                "qmljsicons.cpp", "qmljsicons.h",
                "qmljsicontextpane.h",
                "qmljsimportdependencies.cpp", "qmljsimportdependencies.h",
                "qmljsindenter.cpp", "qmljsindenter.h",
                "qmljsinterpreter.cpp", "qmljsinterpreter.h",
                "qmljsdialect.cpp", "qmljsdialect.h",
                "qmljslineinfo.cpp", "qmljslineinfo.h",
                "qmljslink.cpp", "qmljslink.h",
                "qmljsmodelmanagerinterface.cpp", "qmljsmodelmanagerinterface.h",
                "qmljsplugindumper.cpp", "qmljsplugindumper.h",
                "qmljspropertyreader.cpp", "qmljspropertyreader.h",
                "qmljsqrcparser.cpp", "qmljsqrcparser.h",
                "qmljsreformatter.cpp", "qmljsreformatter.h",
                "qmljsrewriter.cpp", "qmljsrewriter.h",
                "qmljsscanner.cpp", "qmljsscanner.h",
                "qmljsscopeastpath.cpp", "qmljsscopeastpath.h",
                "qmljsscopebuilder.cpp", "qmljsscopebuilder.h",
                "qmljsscopechain.cpp", "qmljsscopechain.h",
                "qmljssimplereader.cpp", "qmljssimplereader.h",
                "qmljsstaticanalysismessage.cpp", "qmljsstaticanalysismessage.h",
                "qmljstypedescriptionreader.cpp", "qmljstypedescriptionreader.h",
                "qmljsutils.cpp", "qmljsutils.h",
                "qmljsvalueowner.cpp", "qmljsvalueowner.h",
                "qmljsviewercontext.cpp", "qmljsviewercontext.h"
            ]
        }

        Group {
            name: "Parser"
            prefix: "parser/"
            files: [
                "qmldirparser.cpp", "qmldirparser_p.h",
                "qmlerror.cpp", "qmlerror.h",
                "qmljsast.cpp", "qmljsast_p.h",
                "qmljsastfwd_p.h",
                "qmljsastvisitor.cpp", "qmljsastvisitor_p.h",
                "qmljsengine_p.cpp", "qmljsengine_p.h",
                "qmljsglobal_p.h",
                "qmljsgrammar.cpp", "qmljsgrammar_p.h",
                "qmljskeywords_p.h",
                "qmljslexer.cpp", "qmljslexer_p.h",
                "qmljsmemorypool_p.h",
                "qmljsparser.cpp", "qmljsparser_p.h",
            ]
        }

        Export {
            Depends { name: "CPlusPlus" }
            Depends { name: "LanguageUtils" }
        }
    }
}
