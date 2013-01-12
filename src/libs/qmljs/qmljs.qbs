import qbs.base 1.0
import "../QtcLibrary.qbs" as QtcLibrary

QtcLibrary {
    name: "QmlJS"

    cpp.includePaths: base.concat("parser")
    cpp.defines: base.concat([
        "QMLJS_BUILD_DIR",
        "QT_CREATOR"
    ])
    cpp.optimization: "fast"

    Depends { name: "Utils" }
    Depends { name: "LanguageUtils" }
    Depends { name: "cpp" }
    Depends { name: "Qt"; submodules: ["widgets", "script"] }

    files: [
        "jsoncheck.cpp",
        "jsoncheck.h",
        "qmljs.qrc",
        "qmljs_global.h",
        "qmljsbind.cpp",
        "qmljsbind.h",
        "qmljscheck.cpp",
        "qmljscheck.h",
        "qmljscodeformatter.cpp",
        "qmljscodeformatter.h",
        "qmljscompletioncontextfinder.cpp",
        "qmljscompletioncontextfinder.h",
        "qmljscontext.cpp",
        "qmljscontext.h",
        "qmljsdelta.cpp",
        "qmljsdelta.h",
        "qmljsdocument.cpp",
        "qmljsdocument.h",
        "qmljsevaluate.cpp",
        "qmljsevaluate.h",
        "qmljsicons.cpp",
        "qmljsicons.h",
        "qmljsicontextpane.h",
        "qmljsindenter.cpp",
        "qmljsindenter.h",
        "qmljsinterpreter.cpp",
        "qmljsinterpreter.h",
        "qmljslineinfo.cpp",
        "qmljslineinfo.h",
        "qmljslink.cpp",
        "qmljslink.h",
        "qmljsmodelmanagerinterface.cpp",
        "qmljsmodelmanagerinterface.h",
        "qmljspropertyreader.cpp",
        "qmljspropertyreader.h",
        "qmljsreformatter.cpp",
        "qmljsreformatter.h",
        "qmljsrewriter.cpp",
        "qmljsrewriter.h",
        "qmljsscanner.cpp",
        "qmljsscanner.h",
        "qmljsscopeastpath.cpp",
        "qmljsscopeastpath.h",
        "qmljsscopebuilder.cpp",
        "qmljsscopebuilder.h",
        "qmljsscopechain.cpp",
        "qmljsscopechain.h",
        "qmljsstaticanalysismessage.cpp",
        "qmljsstaticanalysismessage.h",
        "qmljstypedescriptionreader.cpp",
        "qmljstypedescriptionreader.h",
        "qmljsutils.cpp",
        "qmljsutils.h",
        "qmljsvalueowner.cpp",
        "qmljsvalueowner.h",
        "images/element.png",
        "images/func.png",
        "images/property.png",
        "images/publicmember.png",
        "parser/qmldirparser.cpp",
        "parser/qmldirparser_p.h",
        "parser/qmlerror.cpp",
        "parser/qmlerror.h",
        "parser/qmljsast.cpp",
        "parser/qmljsast_p.h",
        "parser/qmljsastfwd_p.h",
        "parser/qmljsastvisitor.cpp",
        "parser/qmljsastvisitor_p.h",
        "parser/qmljsengine_p.cpp",
        "parser/qmljsengine_p.h",
        "parser/qmljsglobal_p.h",
        "parser/qmljsgrammar.cpp",
        "parser/qmljsgrammar_p.h",
        "parser/qmljskeywords_p.h",
        "parser/qmljslexer.cpp",
        "parser/qmljslexer_p.h",
        "parser/qmljsmemorypool_p.h",
        "parser/qmljsparser.cpp",
        "parser/qmljsparser_p.h",
        "persistenttrie.cpp",
        "persistenttrie.h",
        "consolemanagerinterface.cpp",
        "consolemanagerinterface.h",
        "consoleitem.cpp",
        "consoleitem.h",
        "iscriptevaluator.h"
    ]

    ProductModule {
        Depends { name: "cpp" }
        Depends { name: "LanguageUtils" }
        cpp.defines: [
            "QT_CREATOR"
        ]
    }
}

