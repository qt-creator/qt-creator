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
        script: root.currentStateIndex = n;
    }

    // TextInputs don't loose focus automatically when user clicks away, have to be done explicitly
    signal unFocus
    MouseRegion {
        anchors.fill:parent
        hoverEnabled:true
        onExited: root.unFocus();
    }
    onCurrentStateIndexChanged: unFocus();

    // Colors
    SystemPalette { id:systemPalette; }

    Flickable {
        id: listView

        anchors.left:root.left
        anchors.right:root.right
        anchors.top:root.top
        height:statesRow.height+1
        anchors.topMargin:-1;
        anchors.leftMargin:-1;

        viewportHeight: height
        viewportWidth: statesRow.width+2


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
                    y:18
                    id: newStateBoxLoader;
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
                        if (container.x+container.width > listView.viewportX + listView.width)
                            horizontalScrollbar.viewPosition = container.x+container.width - listView.width;
                        if (container.x < listView.viewportX)
                            horizontalScrollbar.viewPosition = container.x;
                    }
                }
            }


            Rectangle {
                id: highlight
                anchors.fill: parent
                border.width:1
                border.color:"black"
                color:parent.isCurrentState?systemPalette.highlight:"#4F4F4F";
                Rectangle {
                    width:parent.width-1
                    x:1
                    y:-30
                    height:45
                    gradient: Gradient {
                        GradientStop { position: 0.0; color: "#FFFFFF" }
                        GradientStop { position: 1.0; color: highlight.color }
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

            MouseRegion {
                id: itemRegion
                anchors.fill: container
                onClicked: {
                    root.unFocus();
                    root.currentStateIndex = index;
                }
            }



            // The erase button
            Rectangle {
                id: removeState

                visible: (index != 0 && root.currentStateIndex==index)

                anchors.right: parent.right
                anchors.top: parent.top
                anchors.topMargin: 7;
                anchors.rightMargin:4;

                width: 12
                height: width

                color: root.currentStateIndex==index?"#AEAEAE":"#707070"
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
                    color:highlight.color;
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

            Connection {
                sender: root
                signal: "unFocus()"
                script: stateNameEditor.unFocus()
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

                onWidthChanged: txt.updateText();

                Text{
                    anchors.top: parent.top
                    anchors.topMargin: 2
                    anchors.horizontalCenter: textLimits.horizontalCenter
                    id: txt
                    color: "#E1E1E1";
                    property string candidateText: stateName
                    onCandidateTextChanged: updateText();
                    function updateText() {
                        var cut=Math.floor(candidateText.length/2);
                        text=candidateText;
                        if (parent.width<=0) return;
                        while (width > parent.width) {
                            cut = cut-1;
                            text = candidateText.substring(0,cut)+"..."+
                                   candidateText.substring(candidateText.length-cut,candidateText.length);
                        }
                    }

                }
                MouseRegion {
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
                        if (visible)
                            statesEditorModel.renameState(index,stateNameInput.text);
                        visible=false;
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
                                statesEditorModel.renameState(index,text);
                                stateNameEditor.visible=false;
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

        visible:(newStateBoxLoader.x+newStateBoxLoader.width/2-11>listView.width+listView.viewportX);


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
            MouseRegion {
                anchors.fill:parent
                onClicked: {
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
            color:"transparent"
            border.width:1;
            border.color:"#8F8F8F";
        }

    }
}
