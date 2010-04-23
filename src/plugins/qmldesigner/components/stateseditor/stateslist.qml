import Qt 4.7

// Shows list of states
// the model has to expose the path through an iconUrl property
Rectangle {
    id: root
    property int currentStateIndex : 0
    signal createNewState
    signal deleteState(int index)
    signal duplicateCurrentState
    signal startRenaming

    color: "#4f4f4f";

    function adjustCurrentStateIndex() {
        if (currentStateIndex >= statesEditorModel.count)
            currentStateIndex = statesEditorModel.count-1;
    }

    Connections {
        target: statesEditorModel
        onCountChanged: adjustCurrentStateIndex()
    }

    Connections {
        target: statesEditorModel
        onChangedToState: root.currentStateIndex = n;
    }

    // TextInputs don't loose focus automatically when user clicks away, have to be done explicitly
    signal unFocus
    MouseArea {
        anchors.fill:parent
        hoverEnabled:true
        onExited: root.unFocus();
    }
    onCurrentStateIndexChanged: {
        if (currentStateIndex <= 0)
            currentStateIndex = 0;
        else
            unFocus();

    }

    Flickable {
        id: listView

        anchors.left:root.left
        anchors.right:root.right
        anchors.top:root.top
        height:statesRow.height+2
        contentHeight: height
        contentWidth: statesRow.width+2


        Row {
            id: statesRow
            spacing:10
            Row {
                id: listViewRow
                Repeater {
                    model: statesEditorModel
                    delegate: delegate
                }
            }
        }

        focus: true;
        clip: true;
        boundsBehavior: Flickable.DragOverBounds;
        interactive:false;

    }

    Component {
        id: delegate
        Item {
            id: container
            width:img.width+32
            height: img.height + txt.height + 29

            property bool isCurrentState: root.currentStateIndex == index;
            onXChanged: scrollBarAdjuster.adjustScrollBar();
            onIsCurrentStateChanged: scrollBarAdjuster.adjustScrollBar();

            Item {
                id:scrollBarAdjuster
                function adjustScrollBar() {
                    if (parent.isCurrentState) {
                        if (container.x+container.width > listView.contentX + listView.width)
                            horizontalScrollbar.viewPosition = container.x+container.width - listView.width;
                        if (container.x < listView.contentX)
                            horizontalScrollbar.viewPosition = container.x;
                    }
                }
            }

            Connections {
                target: root
                onStartRenaming: if (root.currentStateIndex == index) startRenaming();
            }

            function startRenaming() {
                stateNameInput.text=stateName;
                stateNameInput.focus=true;
                stateNameEditor.visible=true;
                stateNameInput.cursorVisible=true;
                stateNameInput.selectAll();
            }


            Loader {
                sourceComponent: underlay
                anchors.fill: parent
                property variant color: parent.isCurrentState?highlightColor:"#4F4F4F";
            }

            Item {
                id: img

                width:100
                height:100
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.top: textLimits.bottom
                anchors.topMargin: 16
                Image {
                    anchors.centerIn:parent
                    pixmap: statePixmap
                    Rectangle {
                        anchors.fill:parent
                        color:"transparent"
                        border.width:1
                        border.color:"black"
                    }
                }
            }

            MouseArea {
                id: itemRegion
                anchors.fill: container
                onClicked: {
                    root.unFocus();
                    root.currentStateIndex = index;
                }
            }

            Connections {
                target: root
                onUnFocus: stateNameEditor.unFocus();
            }

            Item {
                id: textLimits
                anchors.top: parent.top
                anchors.topMargin:4
                anchors.left:parent.left
                anchors.right:parent.right
                anchors.leftMargin:4
                anchors.rightMargin:4
                height: txt.height
                clip: false
                Text {
                    anchors.top: parent.top
                    anchors.topMargin: 2
                    anchors.horizontalCenter: textLimits.horizontalCenter
                    id: txt
                    color: root.currentStateIndex==index ? "white" : "#E1E1E1";
                    text: stateName
                    width:parent.width
                    elide:Qt.ElideMiddle
                    horizontalAlignment:Qt.AlignHCenter
                }
                MouseArea {
                    id: txtRegion
                    anchors.fill:parent
                    onClicked: {
                        if (root.currentStateIndex != index)
                            root.unFocus();
                        root.currentStateIndex = index;
                    }
                    onDoubleClicked: if (index!=0) {
                        startRenaming();
                    }
                }

                Rectangle {
                    id:stateNameEditor
                    visible:false
                    // force update
                    onVisibleChanged: stateNameInput.updateScroll();

                    x:2-parent.anchors.leftMargin
                    y:2
                    height:parent.height
                    width:container.width-3
                    clip:true

                    color:"white"
                    border.width:2
                    border.color:"#4f4f4f"
                    radius:4
                    function unFocus() {
                        if (visible) {
                            visible=false;
                            statesEditorModel.renameState(index, stateNameInput.text);
                        }
                    }

                    Text {
                        text:stateNameInput.text
                        visible:false
                        id:textMetric
                    }
                    Text {
                        visible:false
                        id:cursorMetric
                    }


                    Item {
                        x:6
                        width:parent.width-10
                        height:parent.height
                        clip:true

                        TextInput {
                            id:stateNameInput
                            text:stateName
                            width:Math.max(textMetric.width+4, parent.width)
                            onCursorPositionChanged: updateScroll();
                            function updateScroll() {
                                cursorMetric.text=text.substring(0,cursorPosition);
                                var cM = cursorPosition>0?cursorMetric.width:0;
                                if (cM+4+x>parent.width)
                                    x = parent.width - cM - 4;
                                cursorMetric.text=text.substring(0,cursorPosition-1);
                                var cM = cursorPosition>1?cursorMetric.width:0;
                                if (cM+x<0)
                                    x = -cM;
                            }
                            onAccepted: {
                                if (stateNameEditor.visible) {
                                    stateNameEditor.visible = false;
                                    statesEditorModel.renameState(index,text);
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    Component {
        id: underlay
        Item {
            anchors.fill:parent
            property variant color: parent.color
            clip:true
            Rectangle {
                width:parent.width
                height:parent.height
                y:-parent.height/2
                gradient: Gradient {
                    GradientStop { position: 0.0; color: Qt.lighter(parent.color) }
                    GradientStop { position: 1.0; color: parent.color }
                }
            }
            Rectangle {
                width:parent.width
                height:parent.height
                y:parent.height/2
                gradient: Gradient {
                    GradientStop { position: 0.0; color: parent.color }
                    GradientStop { position: 1.0; color: Qt.darker(parent.color) }
                }
            }

            Rectangle {
                anchors.top:parent.top
                anchors.left:parent.left
                width:parent.width-1
                height:1
                color: Qt.lighter(parent.color)
            }
            Rectangle {
                anchors.bottom:parent.bottom
                anchors.left:parent.left
                anchors.leftMargin:1
                width:parent.width-1
                height:1
                color: Qt.darker(parent.color)
            }
            Rectangle {
                anchors.top:parent.top
                anchors.left:parent.left
                width:1
                height:parent.height-1
                gradient: Gradient {
                    GradientStop { position: 0.0; color: Qt.lighter(parent.color) }
                    GradientStop { position: 1.0; color: parent.color }
                }
            }
            Rectangle {
                anchors.top:parent.top
                anchors.right:parent.right
                anchors.topMargin:1
                width:1
                height:parent.height-1
                gradient: Gradient {
                    GradientStop { position: 0.0; color: parent.color }
                    GradientStop { position: 1.0; color: Qt.darker(parent.color) }
                }
            }
        }
    }

    Rectangle {
        id: addStateButton
        color: "#4f4f4f"
        width: addStateText.width
        height: addStateText.height+3
        anchors.left: listView.left
        anchors.bottom: root.bottom

        Loader {
            sourceComponent: underlay
            anchors.fill: parent
            property variant color: "#4f4f4f"
        }

        Rectangle {
            id: addStatePressedBorder
            anchors.fill:parent
            anchors.bottomMargin:1
            anchors.rightMargin:1
            border.width:1
            border.color:"black"
            color:"transparent"
            visible:false
        }

        states: [
            State {
               name: "Pressed"
               PropertyChanges {
                   target: addStatePressedBorder
                   visible:true
               }
               PropertyChanges {
                   target: addStateText
                   color: "#1c1c1c"
               }
            },
            State {
                name: "Disabled"
                PropertyChanges {
                    target: addStateText
                    color: "#5f5f5f"
                }
            }
        ]

        Text {
            id: addStateText
            text: " Clone "
            color: "#cfcfcf"
            font.pixelSize:10
            y:1
        }
        MouseArea {
            anchors.fill: parent
            onPressed: if (parent.state != "Disabled") parent.state = "Pressed";
            onReleased: if (parent.state == "Pressed") {
                parent.state = "";
                // force close textinput
                root.unFocus();
                if (root.currentStateIndex == 0)
                    root.createNewState(); //create new state
                else
                    root.duplicateCurrentState(); //duplicate current state
                // select the new state
                root.currentStateIndex = statesEditorModel.count - 1;
            }
        }
    }

    Rectangle {
        id: renameStateButton
        color: "#4f4f4f"
        border.color: "black"
        border.width: 1
        width: renameStateText.width
        height: renameStateText.height+3
        anchors.left: addStateButton.right
        anchors.bottom: root.bottom

        Loader {
            sourceComponent: underlay
            anchors.fill: parent
            property variant color: "#4f4f4f"
        }

        Rectangle {
            id: renameStatePressedBorder
            anchors.fill:parent
            anchors.bottomMargin:1
            anchors.rightMargin:1
            border.width:1
            border.color:"black"
            color:"transparent"
            visible:false
        }

        states: [
            State {
               name: "Pressed"
               PropertyChanges {
                   target: renameStatePressedBorder
                   visible:true
               }
            PropertyChanges {
                   target: renameStateText
                   color: "#1c1c1c"
               }
            },
            State {
                name: "Disabled"
                when: root.currentStateIndex == 0;
                PropertyChanges {
                    target: renameStateText
                    color: "#5f5f5f"
                }
            }
        ]

        Text {
            id: renameStateText
            text: " Rename "
            color: "#cfcfcf"
            font.pixelSize:10
            y:1
        }
        MouseArea {
            anchors.fill: parent
            onPressed: if (parent.state != "Disabled") parent.state = "Pressed";
            onReleased: if (parent.state == "Pressed") {
                parent.state = "";
                root.startRenaming();
            }
        }
    }

    Rectangle {
        id: removeStateButton
        color: "#4f4f4f"
        border.color: "black"
        border.width: 0
        width: removeStateText.width
        height: removeStateText.height+3
        anchors.left: renameStateButton.right
        anchors.bottom: root.bottom

        Loader {
            sourceComponent: underlay
            anchors.fill: parent
            property variant color: "#4f4f4f"
        }

        Rectangle {
            id: removeStatePressedBorder
            anchors.fill:parent
            anchors.bottomMargin:1
            anchors.rightMargin:1
            border.width:1
            border.color:"black"
            color:"transparent"
            visible:false
        }

        states: [
            State {
                name: "Pressed"
                PropertyChanges {
                    target: removeStatePressedBorder
                    visible:true
                }
                PropertyChanges {
                    target: removeStateText
                    color: "#1c1c1c"
                }
            },
            State {
                name: "Disabled"
                when: root.currentStateIndex == 0;
                PropertyChanges {
                    target: removeStateText
                    color: "#5f5f5f"
                }
            }
        ]

        Text {
            id: removeStateText
            text: " Delete "
            color: "#cfcfcf"
            font.pixelSize:10
            y:1
        }
        MouseArea {
            anchors.fill: parent
            onPressed: {
                if (parent.state != "Disabled") {
                    parent.state = "Pressed";
                }
            }
            onReleased: {
                if (parent.state == "Pressed") {
                    parent.state = "";
                    root.unFocus();
                    root.deleteState(root.currentStateIndex);
                    horizontalScrollbar.contentSizeDecreased();
                }
            }
        }
    }

    HorizontalScrollBar {
        id: horizontalScrollbar

        flickable: listView
        anchors.left: removeStateButton.right
        anchors.right : listView.right
        anchors.top : listView.bottom
        anchors.topMargin: 0
        anchors.rightMargin: 1
        height: 10
        onUnFocus: root.unFocus();
    }

}
