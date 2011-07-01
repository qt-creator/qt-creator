import QtQuick 1.0
import components 1.0 as Components

Item {
    id: exampleBrowserRoot
    Item {
        id : lineEditRoot
        width: parent.width
        height: lineEdit.height

        Components.TextField {
            Behavior on width { NumberAnimation{} }
            placeholderText: !checkBox.checked ? qsTr("Search in Tutorials") : qsTr("Search in Tutorials, Examples and Demos")
            focus: true
            id: lineEdit
            width: lineEditRoot.width - checkBox.width - 20 - tagFilterButton.width
            onTextChanged: examplesModel.filterRegExp = RegExp('.*'+text, "im")
        }

        Components.CheckBox {
            id: checkBox
            text: qsTr("Show Examples and Demos")
            checked: false
            anchors.leftMargin: 6
            anchors.left: lineEdit.right
            anchors.verticalCenter: lineEdit.verticalCenter
            height: lineEdit.height
            onCheckedChanged: examplesModel.showTutorialsOnly = !checked;
        }

        Components.Button {
            id: tagFilterButton
            property string tag
            Behavior on width { NumberAnimation{} }
            onTagChanged: { examplesModel.filterTag = tag; examplesModel.updateFilter() }
            anchors.leftMargin: 6
            anchors.left: checkBox.right
            anchors.verticalCenter: lineEdit.verticalCenter
            visible: !examplesModel.showTutorialsOnly
            text: tag === "" ? qsTr("Filter by Tag") : qsTr("Tag Filter: %1").arg(tag)
            onClicked: {
                tagBrowserLoader.source = "TagBrowser.qml"
                tagBrowserLoader.item.visible = true
            }
        }
    }
    Components.ScrollArea  {
        id: scrollArea
        anchors.topMargin: lineEditRoot.height
        anchors.fill: parent
        clip: true
        frame: false
        Column {
            Repeater {
                id: repeater
                model: examplesModel
                delegate: ExampleDelegate { width: scrollArea.width-20 }
            }
        }
    }

    Loader {
        id: tagBrowserLoader
        anchors.fill: parent
    }
}
