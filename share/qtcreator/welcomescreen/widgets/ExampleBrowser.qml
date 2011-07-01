import QtQuick 1.0
import components 1.0 as Components

Item {
    id: exampleBrowserRoot
    Item {
        id : lineEditRoot
        width: parent.width
        height: lineEdit.height

        Connections {
            target: gettingStarted
            onTagsUpdated: {
                var tagList = gettingStarted.tagList()
                for (var tag in tagList) {
                    tagsTestModel.append({ "text": tagList[tag], "value": tagList[tag] });
                }
            }
        }

        Components.TextField {
            Behavior on width { NumberAnimation{} }
            placeholderText: !checkBox.checked ? qsTr("Search in Tutorials") : qsTr("Search in Tutorials, Examples and Demos")
            focus: true
            id: lineEdit
            width: lineEditRoot.width - checkBox.width - 20 - tagFilterButton.width
            onTextChanged: examplesModel.filterRegExp = RegExp('.*'+text, "im")
        }

        ListModel {
            id: tagsTestModel
        }

        Components.CheckBox {
            id: checkBox
            text: qsTr("Show Examples and Demos")
            checked: false
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
            anchors.left: checkBox.right
            anchors.verticalCenter: lineEdit.verticalCenter
            visible: !examplesModel.showTutorialsOnly
            text: tag === "" ? qsTr("Filter by Tag") : qsTr("Tag Filter: %1").arg(tag)
            onClicked: tagChooser.visible = !tagChooser.visible
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


    Rectangle {
        id: tagChooser
        anchors.fill: parent
        color: "darkgrey"
        visible: false
        opacity: 0.95
        radius: 6
        MouseArea { anchors.fill: parent; hoverEnabled: true } // disable mouse on background
        Text {
            id: descr;
            anchors.margins: 6;
            color: "white";
            text: qsTr("Please choose a tag to filter for:");
            anchors.top: parent.top;
            anchors.left: parent.left
            font.bold: true
        }

        Item {
            width: rect.width
            height: rect.height

            anchors.margins: 6;
            anchors.top: parent.top;
            anchors.right: parent.right

            Rectangle {
                color: "red"
                id: rect
                radius: 4
                opacity: 0.3
                width: clearText.width+4
                height: clearText.height+4
                x: clearText.x-2
                y: clearText.y-2
            }
            Text { id: clearText; text: qsTr("Clear"); color: "white"; anchors.centerIn: parent }
            MouseArea {
                id: mouseArea
                anchors.fill: parent
                hoverEnabled: true
                onClicked: {
                    tagChooser.visible = false;
                    tagFilterButton.tag = "";
                }
            }
        }

        Flickable {
            id: flickable
            anchors.fill: parent
            anchors.margins: 6
            anchors.topMargin: descr.height + anchors.margins*2
            contentHeight: flow.height
            contentWidth: flow.width
            flickableDirection: Flickable.VerticalFlick
            clip: true
            Flow {
                width: tagChooser.width
                id: flow
                spacing: 6
                Repeater {
                    model: tagsTestModel
                    delegate: Item {
                        width: btnRect.width
                        height: btnRect.height
                        Rectangle {
                            id: btnRect
                            radius: 4
                            opacity: 0
                            width: text.width+4
                            height: text.height+4
                            x: text.x-2
                            y: text.y-2
                        }
                        Text { id: text; text: model.text; color: "white"; anchors.centerIn: parent }
                        MouseArea {
                            id: mouseArea
                            anchors.fill: parent
                            hoverEnabled: true
                        }

                        states: [
                            State {
                                name: "selected"
                                when: mouseArea.pressed
                            },
                            State {
                                name: "hovered"
                                when: mouseArea.containsMouse
                                PropertyChanges {
                                    target: btnRect
                                    color: "darkblue"
                                    opacity: 0.3
                                }
                            }
                        ]
                        transitions: [
                            Transition {
                                from: "hovered"
                                to: "selected"
                                ParallelAnimation {
                                    PropertyAction { target: tagFilterButton; property: "tag"; value: model.value }
                                    PropertyAction { target: tagChooser; property: "visible"; value: false }
                                    ColorAnimation { to: "#00000000"; duration: 0 }
                                }
                            }
                        ]
                    }
                }
            }

        }
    }
}
