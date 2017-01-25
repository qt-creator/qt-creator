import qbs

CppApplication {
    type: "application"
    consoleApplication: true
    name: "%{ProjectName}"
@if "%{TestFrameWork}" == "QtTest"
@if "%{RequireGUI}" == "true"

    Depends { name: "Qt.core" }
    Depends { name: "Qt.gui" }
    Depends {
        name: "Qt.widgets"
        condition: Qt.core.versionMajor > 4
    }
@else

    Depends { name: "Qt.core" }
@endif
@endif

    cpp.cxxLanguageVersion: "c++11"

    files: [
        "%{MainCppName}"
    ]
}
