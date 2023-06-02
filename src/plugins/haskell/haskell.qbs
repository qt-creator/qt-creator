import qbs 1.0

QtcPlugin {
    name: "Haskell"

    Depends { name: "Qt.widgets" }
    Depends { name: "Utils" }

    Depends { name: "Core" }
    Depends { name: "TextEditor" }
    Depends { name: "ProjectExplorer" }

    files: [
        "haskell.qrc",
        "haskellbuildconfiguration.cpp", "haskellbuildconfiguration.h",
        "haskellconstants.h",
        "haskelleditorfactory.cpp", "haskelleditorfactory.h",
        "haskell_global.h",
        "haskellhighlighter.cpp", "haskellhighlighter.h",
        "haskellmanager.cpp", "haskellmanager.h",
        "haskellplugin.cpp", "haskellplugin.h",
        "haskellproject.cpp", "haskellproject.h",
        "haskellrunconfiguration.cpp", "haskellrunconfiguration.h",
        "haskellsettings.cpp", "haskellsettings.h",
        "haskelltokenizer.cpp", "haskelltokenizer.h",
        "stackbuildstep.cpp", "stackbuildstep.h"
    ]
}
