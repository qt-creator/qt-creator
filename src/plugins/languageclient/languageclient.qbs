import qbs 1.0

QtcPlugin {

    name: "LanguageClient"

    Depends { name: "Qt.core" }
    Depends { name: "Qt.testlib"; condition: qtc.withPluginTests }

    Depends { name: "Utils" }
    Depends { name: "ProjectExplorer" }
    Depends { name: "LanguageServerProtocol" }

    Depends { name: "Core" }
    Depends { name: "TextEditor" }

    files: [
        "callhierarchy.cpp",
        "callhierarchy.h",
        "client.cpp",
        "client.h",
        "clientrequest.cpp",
        "clientrequest.h",
        "currentdocumentsymbolsrequest.cpp",
        "currentdocumentsymbolsrequest.h",
        "diagnosticmanager.cpp",
        "diagnosticmanager.h",
        "documentsymbolcache.cpp",
        "documentsymbolcache.h",
        "dynamiccapabilities.cpp",
        "dynamiccapabilities.h",
        "languageclient.qrc",
        "languageclient_global.h", "languageclienttr.h",
        "languageclientformatter.cpp",
        "languageclientformatter.h",
        "languageclienthoverhandler.cpp",
        "languageclienthoverhandler.h",
        "languageclientfunctionhint.cpp",
        "languageclientfunctionhint.h",
        "languageclientinterface.cpp",
        "languageclientinterface.h",
        "languageclientcompletionassist.cpp",
        "languageclientcompletionassist.h",
        "languageclientmanager.cpp",
        "languageclientmanager.h",
        "languageclientoutline.cpp",
        "languageclientoutline.h",
        "languageclientplugin.cpp",
        "languageclientplugin.h",
        "languageclientquickfix.cpp",
        "languageclientquickfix.h",
        "languageclientsettings.cpp",
        "languageclientsettings.h",
        "languageclientsymbolsupport.cpp",
        "languageclientsymbolsupport.h",
        "languageclientutils.cpp",
        "languageclientutils.h",
        "locatorfilter.cpp",
        "locatorfilter.h",
        "lspinspector.cpp",
        "lspinspector.h",
        "progressmanager.cpp",
        "progressmanager.h",
        "semantichighlightsupport.cpp",
        "semantichighlightsupport.h",
        "snippet.cpp",
        "snippet.h",
    ]

    Properties {
        condition: qbs.toolchain.contains("mingw")
        cpp.cxxFlags: "-Wa,-mbig-obj"
    }

    Export { Depends { name: "LanguageServerProtocol" } }
}
