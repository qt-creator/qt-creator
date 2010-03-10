import Qt 4.6

// Shows list of states
// the model has to expose the path through an iconUrl property
Rectangle {
    id: root
    property int currentStateIndex : 0
    signal createNewState
    signal deleteState(int index)
    signal duplicateCurrentState

    color: "#707070";

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
        onChangedToState: { root.currentStateIndex = n; }
    }

    // TextInputs don't loose focus automatically when user clicks away, have to be done explicitly
    signal unFocus
    MouseArea {
        anchors.fill:parent
        hoverEnabled:true
        onExited: root.unFocus();
    }
    onCurrentStateIndexChanged: {
        if (currentStateIndex<0) currentStateIndex=0; else unFocus();
    }

    Flickable {
        id: listView

        anchors.left:root.left
        anchors.right:root.right
        anchors.top:root.top
        height:statesRow.height+2
        anchors.topMargin:-1;
        anchors.leftMargin:-1;

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
            Item {
                id: newStateBoxLoader;
                width:132
                height:listViewRow.height
                Loader {
                    sourceComponent: newStateBox;
                    // make it square
                    width: 100
                    height: 100
                    anchors.horizontalCenter:parent.horizontalCenter
                    anchors.bottom:parent.bottom
                    anchors.bottomMargin:12
                }
            }


        }

        focus: true;
        clip: true;
        overShoot: false;
        interactive:false;

    }

    Component {
        id: delegate
        Item {
            id: container
            //width: Math.max(img.width, txt.width+2) + 32
            width:img.width+32
            height: img.height + txt.height + 32

            property bool isCurrentState:root.currentStateIndex==index;
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


            Rectangle {
                id: highlight
                anchors.fill: parent
                color:parent.isCurrentState?highlightColor:"#4F4F4F";
                clip:true
                Rectangle {
                    width:parent.width
                    height:parent.height
                    y:-parent.height/2
                    gradient: Gradient {
                        GradientStop { position: 0.0; color: Qt.lighter(highlight.color) }
                        GradientStop { position: 1.0; color: highlight.color }
                    }
                }
                Rectangle {
                    width:parent.width
                    height:parent.height
                    y:parent.height/2
                    gradient: Gradient {
                        GradientStop { position: 0.0; color: highlight.color }
                        GradientStop { position: 1.0; color: Qt.darker(highlight.color) }
                    }
                }
            }
            Item {
                id: highlightBorders
                anchors.fill:parent
                anchors.topMargin:1
                Rectangle {
                    anchors.top:parent.top
                    anchors.left:parent.left
                    width:parent.width-1
                    height:1
                    color: Qt.lighter(highlight.color)
                }
                Rectangle {
                    anchors.bottom:parent.bottom
                    anchors.left:parent.left
                    anchors.leftMargin:1
                    width:parent.width-1
                    height:1
                    color: Qt.darker(highlight.color)
                }
                Rectangle {
                    anchors.top:parent.top
                    anchors.left:parent.left
                    width:1
                    height:parent.height-1
                    gradient: Gradient {
                        GradientStop { position: 0.0; color: Qt.lighter(highlight.color) }
                        GradientStop { position: 1.0; color: highlight.color }
                    }
                }
                Rectangle {
                    anchors.top:parent.top
                    anchors.right:parent.right
                    anchors.topMargin:1
                    width:1
                    height:parent.height-1
                    gradient: Gradient {
                        GradientStop { position: 0.0; color: highlight.color }
                        GradientStop { position: 1.0; color: Qt.darker(highlight.color) }
                    }
                }
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


            // The erase button
            Item {
                id: removeState

                visible: (index != 0 && root.currentStateIndex==index)

                anchors.right: parent.right
                anchors.top: parent.top
                anchors.topMargin: 7;
                anchors.rightMargin:4;

                width: 12
                height: width

                states: State{
                    name: "Pressed";
                    PropertyChanges {
                        target: removeState
                        buttonColor: buttonColorDown
                    }
                }

                property var buttonColorUp: "#E1E1E1"
                property var buttonColorDown: Qt.darker(buttonColorUp)
                property var buttonColor: buttonColorUp

                Item {
                    width:parent.width
                    height:parent.height/2 - 1
                    clip: true
                    Rectangle {
                        color: removeState.buttonColor
                        width: removeState.width
                        height: removeState.height
                        radius: width/2
                    }
                }
                Item {
                    width:parent.width
                    height:parent.height/2 - 1
                    y:parent.height/2+1
                    clip: true
                    Rectangle {
                        color: removeState.buttonColor
                        width: removeState.width
                        height: removeState.height
                        radius: width/2
                        y:-parent.y
                    }
                }
                Item {
                    width:2
                    height:parent.height
                    clip: true
                    Rectangle {
                        color: removeState.buttonColor
                        width: removeState.width
                        height: removeState.height
                        radius: width/2
                    }
                }
                Item {
                    width:2
                    height:parent.height
                    x:parent.width-2
                    clip: true
                    Rectangle {
                        color: removeState.buttonColor
                        width: removeState.width
                        height: removeState.height
                        radius: width/2
                        x: -parent.x
                    }
                }

                MouseArea {
                    anchors.fill:parent
                    onClicked: {
                        root.unFocus();

                        root.deleteState(index);
                        horizontalScrollbar.totalLengthDecreased();
                    }
                    onPressed: {parent.state="Pressed"}
                    onReleased: {parent.state=""}

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
                anchors.right:removeState.left
                anchors.leftMargin:4
                height: txt.height
                clip: false
                Text {
                    anchors.top: parent.top
                    anchors.topMargin: 2
                    anchors.horizontalCenter: textLimits.horizontalCenter
                    id: txt
                    color: root.currentStateIndex==index?"white":"#E1E1E1";
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
                        stateNameInput.text=stateName;
                        stateNameInput.focus=true;
                        stateNameEditor.visible=true;
                        stateNameInput.cursorVisible=true;
                        stateNameInput.selectAll();
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
                            statesEditorModel.renameState(index,stateNameInput.text);
                        }
                    }

                    // There is no QFontMetrics equivalent in QML
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
                                    stateNameEditor.visible=false;
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

        visible:(newStateBoxLoader.x+newStateBoxLoader.width/2-11>listView.width+listView.contentX);


        Loader {
            sourceComponent: addState
            anchors.fill: parent
        }
    }



    // The add button
    Component {
        id: addState
        Item {
            anchors.fill:parent
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
            }
            MouseArea {
                anchors.fill:parent
                onClicked: {
                    // force close textinput
                    root.unFocus();
                    if (root.currentStateIndex == 0)
                        root.createNewState(); //create new state
                    else
                        root.duplicateCurrentState(); //duplicate current state
                    // select the new state
                    root.currentStateIndex = statesEditorModel.count - 1;
                }
                onPressed: {parent.state="Pressed"}
                onReleased: {parent.state=""}
            }
        }
    }

    Item {
        id: horizontalScrollbar
        // Current listView implementation sometimes has negative width or contentWidth
        property int viewPosition: 0;
        property int viewLength: ( listView.width>=0 ? listView.width : 0 );
        property int totalLength: ( listView.contentWidth>=0 ? listView.contentWidth : 0 );


        onViewPositionChanged: listView.contentX=viewPosition;
        onViewLengthChanged: {
            if ((totalLength>viewLength) && (viewPosition > totalLength-viewLength))
                viewPosition = totalLength-viewLength;
        }

        function totalLengthDecreased()  {
            if ((totalLength>viewLength) && (viewPosition > totalLength-viewLength))
                viewPosition = totalLength-viewLength;
        }

        //opacity: viewLength < totalLength;

        anchors.left : listView.left
        anchors.right : listView.right
        anchors.top : listView.bottom
        anchors.topMargin: 1

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

            MouseArea {
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
            color:"transparent"
            border.width:1;
            border.color:"#8F8F8F";
        }

    }
}
