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
        anchors.leftMargin: 2;
        anchors.rightMargin: 2;

        viewportHeight: height
        viewportWidth: statesRow.width+2

        Row {
            id: statesRow
            spacing:4
            Row {
                id: listViewRow
                spacing: 4
                Repeater {
                    model: statesEditorModel
                    delegate: Delegate
                }
            }
            Loader {
                sourceComponent: newStateBox;
                // make it square
                width: height
                height: listViewRow.height - 21
                y:18
                id: newStateBoxLoader;
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
            width: Math.max(img.width, txt.width+removeState.width+2) + 4
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
                anchors.top: textLimits.bottom
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

            Item {
                id: textLimits
                anchors.top: parent.top
                anchors.left:parent.left
                anchors.right:removeState.left
                height: txt.height
                clip: true

                TextInput {
                    anchors.top: parent.top
                    anchors.topMargin: 2
                    anchors.horizontalCenter: textLimits.horizontalCenter
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

            // The erase button
            Rectangle {
                id: removeState

                visible: { index != 0 }

                anchors.right: parent.right
                anchors.top: parent.top
                anchors.topMargin: 4;
                anchors.rightMargin:2;

                width: 12
                height: width

                color: "#AEAEAE"
                radius: 6

                // "clicked" overlay
                Rectangle {
                    anchors.fill:parent
                    opacity:parent.state=="Pressed"
                    color: "#909090"
                    radius: parent.radius
                }

                states: State{ name: "Pressed"; }

                // "minus" sign
                Rectangle {
                    width: parent.width - 4;
                    height:2
                    color:(root.currentStateIndex==index?Highlight.color:root.color);
                    anchors.centerIn:parent
                }



                MouseRegion {
                    anchors.fill:parent
                    onClicked: {
                        var oldindex = root.currentStateIndex;
                        root.currentStateIndex = index;
                        root.unFocus();
                        root.deleteCurrentState();
                        root.currentStateIndex = oldindex;
                        if (root.currentStateIndex >= statesEditorModel.count)
                            root.currentStateIndex = root.currentStateIndex-1;
                        horizontalScrollbar.totalLengthDecreased();
                    }
                    onPressed: {parent.state="Pressed"}
                    onReleased: {parent.state=""}

                }
            }
        }
    }


    Component {
        id: newStateBox
        Rectangle {
            anchors.fill: parent
            color: "transparent"
            border.width: 1
            border.color: "#4F4F4F"

            Loader {
                sourceComponent: addState
                anchors.fill: parent

            }
        }
    }

    Rectangle {
        id: floatingNewStateBox

        color: root.color
        border.width: 1
        border.color: "#4F4F4F"
        width: 40
        height: 40
        anchors.bottom:horizontalScrollbar.top
        anchors.right:root.right
        anchors.bottomMargin:1
        anchors.rightMargin:1

        visible:(newStateBoxLoader.x+newStateBoxLoader.width/2-11>listView.width+listView.viewportX);


        Loader {
            sourceComponent: addState
            anchors.fill: parent
        }
    }



   // The add button
   Component {
       id: addState
       Rectangle {

            anchors.centerIn: parent

            width: 21
            height: width

            color:"#4F4F4F"
            radius: width/2

            // "clicked" overlay
            Rectangle {
                anchors.fill:parent
                opacity:parent.state=="Pressed"
                color : "#282828"
                radius: parent.radius
            }

            states: State{ name: "Pressed"; }

            // "plus" sign
            Rectangle {
                width:parent.width-10
                height:3
                color:root.color
                anchors.centerIn:parent
            }
            Rectangle {
                width:3
                height:parent.height-10
                color:root.color
                anchors.centerIn:parent
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
            width: if (horizontalScrollbar.totalLength>0) {
                if (horizontalScrollbar.viewLength>horizontalScrollbar.totalLength) parent.width;
                else horizontalScrollbar.viewLength/horizontalScrollbar.totalLength  * parent.width;
            } else 0;
            height: parent.height;
            x: (horizontalScrollbar.totalLength>0?horizontalScrollbar.viewPosition*horizontalScrollbar.width/horizontalScrollbar.totalLength:0);


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
