pragma Singleton
import QtQuick

QtObject {
    readonly property int width: %{ScreenWidth}
    readonly property int height: %{ScreenHeight}


    readonly property color backgroundColor: "#e8e8e8"
}
