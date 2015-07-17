import QtQuick %{QtQuickVersion}
import QtQuick.Window %{QtQuickWindowVersion}

Window {
    visible: true

@if %{IsUiFileInUse}
    MainForm {
        anchors.fill: parent
        mouseArea.onClicked: {
            Qt.quit();
        }
    }
@else
    MouseArea {
        anchors.fill: parent
        onClicked: {
            Qt.quit();
        }
    }

    Text {
        text: qsTr("Hello World")
        anchors.centerIn: parent
    }
@endif
}
