import Qt 4.6

// Shows list of states
// the model has to expose the path through an iconUrl property
Rectangle {
    id: root
    property int currentStateIndex : 0
    signal createNewState
    signal deleteCurrentState
    signal duplicateCurrentState

    color: "#707070";

    function adjustCurrentStateIndex() {
        if (currentStateIndex >= statesEditorModel.count)
            currentStateIndex = statesEditorModel.count-1;
    }

    Connection {
        sender: statesEditorModel
        signal: "countChanged()"
        script: adjustCurrentStateIndex();
    }

    Connection {
        sender: statesEditorModel
        signal: "changedToState(n)"
        script: {root.currentStateIndex = n}
    }

    // TextInputs don't loose focus automatically when user clicks away, have to be done explicitly
    signal unFocus
    Item { id:focusStealer }

    Flickable {
        id: listView

        anchors.fill: root;

        anchors.bottomMargin: 10;
        anchors.topMargin: 2;
        anchors.leftMargin: 29;
        anchors.rightMargin: 40;

        viewportHeight: height
        viewportWidth: listViewRow.width

        Row {
            id: listViewRow
            spacing: 4
            Repeater {
                model: statesEditorModel
                delegate: Delegate
            }
        }

        focus: true;
        clip: true;
        overShoot: false;
        interactive:false;

    }

    Component {
        id: Delegate
        Item {
            id: container
            width: Math.max(img.width, txt.width) + 4
            height: img.height + txt.height + 4

            Rectangle {
                id: Highlight
                anchors.fill: parent
                color: "#0303e0"
                radius: 4
                opacity: (index==root.currentStateIndex);
                onOpacityChanged: if (opacity) {
                    // If the current item becomes selected, check if it is in the viewport
                    if (container.x < listView.viewportX)
                        horizontalScrollbar.viewPosition = container.x;
                    if (container.x+container.width > listView.viewportX + listView.width)
                        horizontalScrollbar.viewPosition = container.x+container.width - listView.width;
                }
            }

            Image {
                id: img
                pixmap: statePixmap
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.top: parent.top
                anchors.topMargin: 2
            }

            MouseRegion {
                id: itemRegion
                anchors.fill: container
                onClicked: {
                    root.currentStateIndex = index
                    root.unFocus();
                }
            }

            Connection {
                sender: root
                signal: "unFocus()"
                script: txt.unFocus()
            }

            TextInput {
                anchors.top: img.bottom
                anchors.horizontalCenter: img.horizontalCenter
                id: txt
                text: stateName
                color: "#E1E1E1";
                function unFocus() {
                    focus=false;
                    focusStealer.focus=true;
                    txtRegion.enabled=true;
                    if (index!=0)
                        statesEditorModel.renameState(index,text);
                }
                onAccepted: unFocus();
                MouseRegion {
                    id: txtRegion
                    anchors.fill:parent
                    onClicked: {
                        root.currentStateIndex = index;
                        root.unFocus();
                    }
                    onDoubleClicked: if (index!=0) {
                        parent.focus=true;
                        enabled=false;
                    }
                }
            }
        }
    }

    // The add button
    Rectangle {
        id: addState

        anchors.left: root.left
        anchors.top: root.top
        anchors.topMargin: 4;
        anchors.leftMargin: 4;

        width: 20
        height: 50

        // Appearance
        gradient: Gradient {
            GradientStop { position: 0.0; color: "#909090" }
            GradientStop { position: 1.0; color: "#AEAEAE" }
        }
        border.width : 1
        border.color : "#4F4F4F"
        radius: 4

        // "clicked" overlay
        Rectangle {
            anchors.fill:parent
            opacity:parent.state=="Pressed"
            gradient: Gradient {
                GradientStop { position: 0.0; color: "#727272" }
                GradientStop { position: 1.0; color: "#909090" }
            }
            border.width : 1
            border.color : "#4F4F4F"
            radius: 4
        }

        states: State{ name: "Pressed"; }

        // "plus" sign
        Rectangle {
            width:12
            height:2
            color:"black"
            anchors.centerIn:parent
            //effect: Blur { blurRadius: 1; }
        }
        Rectangle {
            width:2
            height:12
            color:"black"
            anchors.centerIn:parent
            //effect: Blur { blurRadius: 1; }
        }

        MouseRegion {
            anchors.fill:parent
            onClicked: {
                root.unFocus();
                if (root.currentStateIndex == 0)
                root.createNewState(); //create new state
                else
                root.duplicateCurrentState(); //duplicate current state

                listView.viewportX = horizontalScrollbar.viewPosition;
                // select the newly created state
                root.currentStateIndex=statesEditorModel.count - 1;
            }
            onPressed: {parent.state="Pressed"}
            onReleased: {parent.state=""}
        }
    }

    // The erase button
    Rectangle {
        id: removeState

        anchors.left: root.left
        anchors.top: addState.bottom
        anchors.topMargin: 4;
        anchors.leftMargin: 4;

        width: 20
        height: 50

        // Appearance
        gradient: Gradient {
            GradientStop { position: 0.0; color: "#909090" }
            GradientStop { position: 1.0; color: "#AEAEAE" }
        }
        border.width : 1
        border.color : "#4F4F4F"
        radius: 4

        // "clicked" overlay
        Rectangle {
            anchors.fill:parent
            opacity:parent.state=="Pressed"
            gradient: Gradient {
                GradientStop { position: 0.0; color: "#727272" }
                GradientStop { position: 1.0; color: "#909090" }
            }
            border.width : 1
            border.color : "#4F4F4F"
            radius: 4
        }

        states: State{ name: "Pressed"; }

        // "minus" sign
        Rectangle {
            width:12
            height:2
            color:"black"
            anchors.centerIn:parent
            //effect: Blur { blurRadius: 1; }
        }

        visible: { root.currentStateIndex != 0 }

        MouseRegion {
            anchors.fill:parent
            onClicked: {
                root.unFocus();
                root.deleteCurrentState();
                if (root.currentStateIndex >= statesEditorModel.count)
                    root.currentStateIndex = root.currentStateIndex-1;
                horizontalScrollbar.totalLengthDecreased();
            }
            onPressed: {parent.state="Pressed"}
            onReleased: {parent.state=""}
            enabled: { root.currentStateIndex != 0 }
        }
    }

    Item {
        id: horizontalScrollbar
        // Current listView implementation sometimes has negative width or viewportWidth
        property int viewPosition: 0;
        property int viewLength: ( listView.width>=0 ? listView.width : 0 );
        property int totalLength: ( listView.viewportWidth>=0 ? listView.viewportWidth : 0 );


        onViewPositionChanged: listView.viewportX=viewPosition;
        onViewLengthChanged: {
            if ((totalLength>viewLength) && (viewPosition > totalLength-viewLength))
                viewPosition = totalLength-viewLength;
        }

        function totalLengthDecreased()  {
            if ((totalLength>viewLength) && (viewPosition > totalLength-viewLength))
                viewPosition = totalLength-viewLength;
        }

        opacity: viewLength < totalLength;

        anchors.left : listView.left
        anchors.right : listView.right
        anchors.bottom : root.bottom
        anchors.bottomMargin: 4

        height: 10;

        // the bar itself
        Item {
            id: draggableBar
            width: if (horizontalScrollbar.viewLength>horizontalScrollbar.totalLength) parent.width;
            else horizontalScrollbar.viewLength/horizontalScrollbar.totalLength  * parent.width;
            height: parent.height;
            x: horizontalScrollbar.viewPosition*horizontalScrollbar.width/horizontalScrollbar.totalLength;


            Rectangle {
                height:parent.height-1
                width:parent.width
                y:1

                gradient: Gradient {
                    GradientStop { position: 0.0; color: "#C6C6C6" }
                    GradientStop { position: 1.0; color: "#7E7E7E" }
                }
            }

            MouseRegion {
                anchors.fill:parent
                property int dragging:0;
                property int originalX:0;
                onPressed: { dragging = 1; originalX = mouse.x;root.unFocus(); }
                onReleased: { dragging = 0; }
                onPositionChanged: if (dragging)
                {
                    var newX = mouse.x - originalX + parent.x;
                    if (newX<0) newX=0;
                    if (newX>horizontalScrollbar.width-draggableBar.width)
                        newX=horizontalScrollbar.width-draggableBar.width;
                    horizontalScrollbar.viewPosition = newX*horizontalScrollbar.totalLength/horizontalScrollbar.width;
                }
            }
        }

        // border
        Rectangle {
            anchors.fill:parent;
            color:"Transparent";
            border.width:1;
            border.color:"#8F8F8F";
        }

    }

}
