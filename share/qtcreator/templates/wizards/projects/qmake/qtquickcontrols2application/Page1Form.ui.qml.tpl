import QtQuick %{QtQuickVersion}
import QtQuick.Controls %{QtQuickControls2Version}
import QtQuick.Layouts %{QtQuickLayoutsVersion}

Item {
    property alias button1: button1
    property alias button2: button2

    RowLayout {
        anchors.centerIn: parent

        Button {
            id: button1
            text: qsTr("Press Me 1")
        }

        Button {
            id: button2
            text: qsTr("Press Me 2")
        }
    }
}
