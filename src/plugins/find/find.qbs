import qbs 1.0

import QtcPlugin

QtcPlugin {
    name: "Find"

    Depends { name: "Core" }

    files: [ "findplugin.cpp" ]
}
