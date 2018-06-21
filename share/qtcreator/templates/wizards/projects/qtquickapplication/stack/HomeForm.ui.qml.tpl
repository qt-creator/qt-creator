import QtQuick %{QtQuickVersion}
import QtQuick.Controls %{QtQuickControlsVersion}

Page {
    width: 600
    height: 400

    title: qsTr("Home")

    Label {
        text: qsTr("You are on the home page.")
        anchors.centerIn: parent
    }
}
