import qbs 1.0
import qbs.FileInfo

import QtcPlugin

QtcPlugin {
    name: "Locator"
    Depends { name: "Core" }

    files: [ "locatorplugin.cpp" ]
}
