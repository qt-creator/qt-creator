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
@if !%{IsQt6}

    cpp.defines: [
        // You can make your code fail to compile if it uses deprecated APIs.
        // In order to do so, uncomment the following line.
        //"QT_DISABLE_DEPRECATED_BEFORE=0x060000" // disables all the APIs deprecated before Qt 6.0.0
    ]
@endif

    files: [
        "%{MainCppFileName}",
@if !%{IsQt6}
        "main.qml",
        "qml.qrc",
@endif
@if %{HasTranslation}
        "%{TsFileName}",
@endif
    ]
    @if %{HasTranslation}

    Group {
        fileTagsFilter: "qm"
        Qt.core.resourcePrefix: "/i18n"
        fileTags: "qt.core.resource_data"
    }
@endif
@if %{IsQt6}
    Group {
        files: ["main.qml"%{AdditionalQmlFilesQbs}]
        Qt.core.resourcePrefix: "%{ProjectName}/"
        fileTags: ["qt.qml.qml", "qt.core.resource_data"]
    }
@endif
}
