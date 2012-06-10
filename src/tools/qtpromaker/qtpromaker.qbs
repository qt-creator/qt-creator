import qbs.base 1.0
import "../QtcTool.qbs" as QtcTool

QtcTool {
    name: "qtpromaker"

    Depends { name: "cpp" }
    Depends { name: "qt"; submodules: ['core'] }

    files: [ "main.cpp" ]
}
