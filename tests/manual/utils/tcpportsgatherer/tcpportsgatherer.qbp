import qbs.base 1.0

Application {
    name: "tcpportsgatherer"

    files: [
        "main.cpp",
        "../../../../src/libs/utils/portlist.cpp",
        "../../../../src/libs/utils/portlist.h",
        "../../../../src/libs/utils/tcpportsgatherer.cpp",
        "../../../../src/libs/utils/tcpportsgatherer.h"
    ]

    cpp.includePaths: [ "../../../../src/libs" ]
    cpp.defines: [ "QTCREATOR_UTILS_STATIC_LIB" ]

    Properties {
        condition: qbs.targetOS == "windows"
        cpp.dynamicLibraries: [ "iphlpapi.lib", "ws2_32.lib" ]
    }

    Depends { name: "cpp" }
    Depends { name: "Qt"; submodules: ["widgets"] }
}
