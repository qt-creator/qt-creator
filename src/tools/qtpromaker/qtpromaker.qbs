import qbs.base 1.0
import QtcTool

QtcTool {
    name: "qtpromaker"

    Depends { name: "Qt.core" }

    files: [ "main.cpp" ]
}
