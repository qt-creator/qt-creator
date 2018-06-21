import QtQuick %{QtQuickVersion}
import QtQuick.Controls %{QtQuickControlsVersion}

Page {
    width: 600
    height: 400

    title: qsTr("Page 2")

    Label {
        text: qsTr("You are on Page 2.")
        anchors.centerIn: parent
    }
}
