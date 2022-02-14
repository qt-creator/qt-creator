import qbs.FileInfo

@if %{IsStatic}
StaticLibrary {
@else
DynamicLibrary {
@endif
@if '%{QtModule}' === 'none'
    Depends { name: "cpp" }
@else
    Depends { name: "Qt.%{QtModule}" }
@endif

    cpp.cxxLanguageVersion: "c++17"
    cpp.defines: [
@if %{IsShared}
        "%{LibraryDefine}",
@endif
@if %{IsQtPlugin}
        "QT_PLUGIN",
@endif

        // You can make your code fail to compile if it uses deprecated APIs.
        // In order to do so, uncomment the following line.
        //"QT_DISABLE_DEPRECATED_BEFORE=0x060000" // disables all the APIs deprecated before Qt 6.0.0
    ]

    files: [
        "%{SrcFileName}",
@if %{IsShared}
        "%{GlobalHdrFileName}",
@endif
        "%{HdrFileName}",
@if %{IsQtPlugin}
        "%{PluginJsonFile}",
@endif
@if %{HasTranslation}
        "%{TsFileName}",
@endif
    ]

@if '%{TargetInstallPath}' != ''
    // Default rules for deployment.
    qbs.installPrefix: ""
    Properties {
        condition: qbs.targetOS.contains("unix")
        install: true
@if %{IsQtPlugin}
        installDir: FileInfo.joinPaths(Qt.core.pluginPath, "%{PluginTargetPath}")
@else
        installDir: "%{TargetInstallPath}"
@endif
    }
@endif
}
