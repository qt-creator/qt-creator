import QtQuick %{QtQuickVersion}
import QtQuick.Controls %{QtQuickControlsVersion}

Page {
    width: 600
    height: 400

    title: qsTr("Page 1")

    Label {
        text: qsTr("You are on Page 1.")
        anchors.centerIn: parent
    }
}
