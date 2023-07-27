import qbs 1.0

QtcPlugin {
    name: "Nim"

    Depends { name: "Qt.widgets" }
    Depends { name: "Utils" }

    Depends { name: "Core" }
    Depends { name: "TextEditor" }
    Depends { name: "ProjectExplorer" }

    cpp.includePaths: base.concat(["."])

    Group {
        name: "General"
        files: [
            "nim.qrc",
            "nimconstants.h",
            "nimplugin.cpp", "nimplugin.h",
            "nimtr.h",
        ]
    }

    Group {
        name: "Editor"
        prefix: "editor/"
        files: [
            "nimeditorfactory.h", "nimeditorfactory.cpp",
            "nimhighlighter.h", "nimhighlighter.cpp",
            "nimindenter.h", "nimindenter.cpp",
            "nimtexteditorwidget.h", "nimtexteditorwidget.cpp",
            "nimcompletionassistprovider.h", "nimcompletionassistprovider.cpp"
        ]
    }

    Group {
        name: "Project"
        prefix: "project/"
        files: [
            "nimbuildsystem.cpp", "nimbuildsystem.h",
            "nimbuildconfiguration.h", "nimbuildconfiguration.cpp",
            "nimcompilerbuildstep.h", "nimcompilerbuildstep.cpp",
            "nimcompilercleanstep.h", "nimcompilercleanstep.cpp",
            "nimoutputtaskparser.h", "nimoutputtaskparser.cpp",
            "nimproject.h", "nimproject.cpp",
            "nimrunconfiguration.h", "nimrunconfiguration.cpp",
            "nimtoolchain.h", "nimtoolchain.cpp",
            "nimblebuildstep.h", "nimblebuildstep.cpp",
            "nimbleproject.h", "nimbleproject.cpp",
            "nimblerunconfiguration.h", "nimblerunconfiguration.cpp",
            "nimbletaskstep.h", "nimbletaskstep.cpp",
            "nimblebuildsystem.h", "nimblebuildsystem.cpp",
            "nimblebuildconfiguration.h", "nimblebuildconfiguration.cpp",
        ]
    }

    Group {
        name: "Settings"
        prefix: "settings/"
        files: [
            "nimcodestylepreferencesfactory.h", "nimcodestylepreferencesfactory.cpp",
            "nimcodestylepreferenceswidget.h", "nimcodestylepreferenceswidget.cpp",
            "nimcodestylesettingspage.h", "nimcodestylesettingspage.cpp",
            "nimsettings.h", "nimsettings.cpp",
        ]
    }

    Group {
        name: "Tools"
        prefix: "tools/"
        files: [
            "nimlexer.h", "nimlexer.cpp",
            "sourcecodestream.h"
        ]
    }

    Group {
        name: "Suggest"
        prefix: "suggest/"
        files: [
            "client.h", "client.cpp",
            "clientrequests.h", "clientrequests.cpp",
            "nimsuggest.h", "nimsuggest.cpp",
            "nimsuggestcache.h", "nimsuggestcache.cpp",
            "server.h", "server.cpp",
            "sexprlexer.h",
            "sexprparser.h",
        ]
    }
}
