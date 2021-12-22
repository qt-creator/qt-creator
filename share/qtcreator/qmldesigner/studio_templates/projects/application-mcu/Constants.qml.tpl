pragma Singleton
import QtQuick 2.15

QtObject {
    readonly property int width: %{ScreenWidth}
    readonly property int height: %{ScreenHeight}


    readonly property color backgroundColor: "#e8e8e8"

    /* DirectoryFontLoader doesn't work with Qt Ultralite.
    However you may want to uncomment this block to load fonts in real qml environment */
    /*
    property alias fontDirectory: directoryFontLoader.fontDirectory
    property alias relativeFontDirectory: directoryFontLoader.relativeFontDirectory

    readonly property font font: Qt.font({
                                             family: Qt.application.font.family,
                                             pixelSize: Qt.application.font.pixelSize
                                         })
    readonly property font largeFont: Qt.font({
                                                  family: Qt.application.font.family,
                                                  pixelSize: Qt.application.font.pixelSize * 1.6
                                              })



    property DirectoryFontLoader directoryFontLoader: DirectoryFontLoader {
        id: directoryFontLoader
    }
    */
}
