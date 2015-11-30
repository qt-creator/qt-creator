import QtQuick %{QtQuickVersion}
import QtQuick.Controls %{QtQuickControlsVersion}
@if %{IsUiFileInUse}
import QtQuick.Dialogs %{QtQuickDialogsVersion}
@endif

ApplicationWindow {
    visible: true
    width: 640
    height: 480
    title: qsTr("Hello World")

    menuBar: MenuBar {
        Menu {
            title: qsTr("File")
            MenuItem {
                text: qsTr("&Open")
                onTriggered: console.log("Open action triggered");
            }
            MenuItem {
                text: qsTr("Exit")
                onTriggered: Qt.quit();
            }
        }
    }

@if %{IsUiFileInUse}
    MainForm {
        anchors.fill: parent
        button1.onClicked: messageDialog.show(qsTr("Button 1 pressed"))
        button2.onClicked: messageDialog.show(qsTr("Button 2 pressed"))
    }

    MessageDialog {
        id: messageDialog
        title: qsTr("May I have your attention, please?")

        function show(caption) {
            messageDialog.text = caption;
            messageDialog.open();
        }
    }
@else
    Label {
        text: qsTr("Hello World")
        anchors.centerIn: parent
    }
@endif
}
