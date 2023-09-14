pragma Singleton
import QtQuick %{QtQuickVersion}
@if %{IsQt6Project}
import QtQuick.Studio.Application
@else
@endif

QtObject {
    readonly property int width: %{ScreenWidth}
    readonly property int height: %{ScreenHeight}

    property string relativeFontDirectory: "fonts"

    /* Edit this comment to add your custom font */
    readonly property font font: Qt.font({
                                             family: Qt.application.font.family,
                                             pixelSize: Qt.application.font.pixelSize
                                         })
    readonly property font largeFont: Qt.font({
                                                  family: Qt.application.font.family,
                                                  pixelSize: Qt.application.font.pixelSize * 1.6
                                              })

    readonly property color backgroundColor: "#EAEAEA"


@if %{IsQt6Project}
    property StudioApplication application: StudioApplication {
        fontPath: Qt.resolvedUrl("../../content/" + relativeFontDirectory)
    }
@else
    property DirectoryFontLoader directoryFontLoader: DirectoryFontLoader {
        id: directoryFontLoader
    }
@endif
}
