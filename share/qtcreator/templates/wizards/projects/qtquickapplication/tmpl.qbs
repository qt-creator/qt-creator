import qbs
CppApplication {
@if "%{UseVirtualKeyboard}" == "true"
    Depends { name: "Qt"; submodules: ["quick", "virtualkeyboard"] }
@else
    Depends { name: "Qt.quick" }
@endif
    install: true
    // Additional import path used to resolve QML modules in Qt Creator's code model
    property pathList qmlImportPaths: []

    files: [
        "%{MainCppFileName}",
    ]

    Group {
        Qt.core.resourcePrefix: "%{ProjectName}/"
        fileTags: ["qt.qml.qml", "qt.core.resource_data"]
        files: ["Main.qml"]
    }
}
