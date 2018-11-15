import qbs

QtcPlugin {
    name: "SerialTerminal"
    condition: Qt.serialport.present

    Depends { name: "Core" }
    Depends { name: "TextEditor" }
    Depends { name: "Utils" }
    Depends { name: "Qt.serialport"; required: false }

    files: [
        "consolelineedit.cpp",
        "consolelineedit.h",
        "serialcontrol.cpp",
        "serialcontrol.h",
        "serialdevicemodel.cpp",
        "serialdevicemodel.h",
        "serialoutputpane.cpp",
        "serialoutputpane.h",
        "serialterminalconstants.h",
        "serialterminalplugin.cpp",
        "serialterminalplugin.h",
        "serialterminalsettings.cpp",
        "serialterminalsettings.h",
    ]
}
