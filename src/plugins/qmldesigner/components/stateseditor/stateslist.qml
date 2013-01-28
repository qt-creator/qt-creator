/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

import QtQuick 1.0

Rectangle {
    id: root

    property int currentStateInternalId : 0
    signal createNewState
    signal deleteState(int nodeId)
    signal duplicateCurrentState
    signal startRenaming

    color: "#4f4f4f";
    property color colorAlpha: "#994f4f4f";

    Connections {
        target: statesEditorModel
        onChangedToState: root.currentStateInternalId = n;
    }

    signal unFocus
    MouseArea {
        anchors.fill:parent
        hoverEnabled:true
        onExited: root.unFocus();
    }
    onCurrentStateInternalIdChanged: {
        if (currentStateInternalId <= 0)
        currentStateInternalId = 0;
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
                Item {
                    id: newStateBoxLoader;
                    width:132
                    height:listViewRow.height
                    Loader {
                        sourceComponent: addState;
                        // make it square
                        width: 100
                        height: 100
                        anchors.horizontalCenter:parent.horizontalCenter
                        anchors.bottom:parent.bottom
                        anchors.bottomMargin:9
                    }
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
            property int baseStateOffset:(index==0?15:0)

            width:img.width+32+baseStateOffset + (index==0?6:0)
            height: img.height + txt.height + 29 //(index==0?29:25)
            //y:(index==0?0:4)

            property bool isCurrentState: root.currentStateInternalId == nodeId;
            onXChanged: scrollBarAdjuster.adjustScrollBar();
            onIsCurrentStateChanged: scrollBarAdjuster.adjustScrollBar();

            Item {
                id:scrollBarAdjuster
                function adjustScrollBar() {
                    if ((parent.isCurrentState) && (container.x+container.width<=listView.contentWidth-floatingNewStateBox.width)) {
                        if (container.x+container.width > listView.contentX + listView.width - floatingNewStateBox.width)
                        horizontalScrollbar.viewPosition = container.x+container.width - listView.width+floatingNewStateBox.width + (index<statesEditorModel.count-1?25:0);
                        if (container.x < listView.contentX)
                        horizontalScrollbar.viewPosition = container.x - (index>0?25:0);
                    }
                }
            }

            Connections {
                target: root
                onStartRenaming: if (root.currentStateInternalId == index) startRenaming();
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
                anchors.rightMargin: index==0?container.baseStateOffset:0
                property variant color: parent.isCurrentState?highlightColor:"#4F4F4F";
            }

            Item {
                id: img

                width:100
                height:100
                anchors.left: parent.left
                anchors.leftMargin: (parent.width - width - container.baseStateOffset)/2
                anchors.bottom: parent.bottom
                anchors.bottomMargin: 9
                Image {
                    anchors.centerIn:parent
                    source: stateImageSource
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
                    root.currentStateInternalId = nodeId;
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
                anchors.right:index==0 ? parent.right : removeState.left
                anchors.leftMargin:4
                anchors.rightMargin:4 + container.baseStateOffset
                height: txt.height
                clip: false
                Text {
                    anchors.top: parent.top
                    anchors.topMargin: 2
                    anchors.horizontalCenter: textLimits.horizontalCenter
                    id: txt
                    color: root.currentStateInternalId==nodeId ? "white" : "#E1E1E1";
                    text: stateName
                    width:parent.width
                    elide:Qt.ElideMiddle
                    horizontalAlignment:Qt.AlignHCenter
                }
                Rectangle {
                    id: textFrame
                    visible:false
                    anchors.fill:parent
                    anchors.topMargin:0
                    anchors.bottomMargin:-4
                    color:"transparent"
                    border.width:2
                    border.color:index!=0 ? highlightColor : "transparent";
                    radius:4
                }
                MouseArea {
                    id: txtRegion
                    anchors.fill:parent
                    onClicked: {
                        if (root.currentStateInternalId != nodeId)
                        root.unFocus();
                        root.currentStateInternalId = nodeId;
                    }
                    onDoubleClicked: if (index!=0) {
                        startRenaming();
                    }
                    hoverEnabled:true
                    onEntered: textFrame.visible=container.isCurrentState;
                    onExited: textFrame.visible=false;
                }

                Rectangle {
                    id:stateNameEditor
                    visible:false
                    onVisibleChanged: stateNameInput.updateScroll();

                    height:parent.height+4
                    width:parent.width
                    clip:true

                    color:"white"
                    border.width:2
                    border.color:"#4f4f4f"
                    radius:4
                    function unFocus() {
                        if (visible) {
                            visible=false;
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
                        y:2
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
                                    statesEditorModel.renameState(nodeId,text);
                                }
                            }
                        }
                    }
                }
            }

            // The erase button
            Item {
                id: removeState

                visible: (index != 0 && root.currentStateInternalId==nodeId)

                anchors.right: parent.right
                anchors.top: parent.top
                anchors.topMargin: 7;
                anchors.rightMargin:4;

                width: 12
                height: width

                states: [
                    State{
                        name: "Pressed";
                        PropertyChanges {
                            target: removeState
                            buttonColor: buttonColorDown
                        }
                        },
                        State{
                            name: "Hover";
                            PropertyChanges {
                                target: removeState
                                buttonColor: buttonColorHover
                            }
                        }
                ]


                property variant buttonColorUp: "#E1E1E1"
                property variant buttonColorDown: Qt.darker(buttonColorUp)
                property variant buttonColor: buttonColorUp
                property variant buttonColorHover: "white"

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

                        root.deleteState(nodeId);
                        horizontalScrollbar.contentSizeDecreased();
                    }
                    onPressed: {parent.state="Pressed"}
                    onReleased: {parent.state=""}
                    hoverEnabled:true
                    onEntered: {parent.state="Hover"}
                    onExited: {parent.state=""}
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

    Item {
        id: floatingNewStateBox
        width:132
        height:listViewRow.height
        anchors.right:root.right

        visible: (newStateBoxLoader.x+newStateBoxLoader.width>=listView.width)

        Rectangle {
            color: root.color
            width:parent.width - 8
            height:parent.height
            anchors.right:parent.right
        }

        Rectangle {
            gradient: Gradient {
                GradientStop { position: 0.0; color: "transparent" }
                GradientStop { position: 0.5; color: root.colorAlpha }
                GradientStop { position: 1.0; color: root.color }
            }
            width:parent.height
            height:8
            rotation:-90
            y : 67
            x : -67
        }


        Loader {
            sourceComponent: addState
            width: 100
            height: 100
            anchors.horizontalCenter:parent.horizontalCenter
            anchors.bottom:parent.bottom
            anchors.bottomMargin:9
        }
    }


    // The add button
    Component {
        id: addState
        Item {
            anchors.fill:parent
            id: addStateBox

            states: [
                State {
                    name:"Hover"
                    PropertyChanges {
                        target:addStateBox
                        buttonColor:hoverColor
                    }
                    },
                    State {
                        name:"Pressed"
                        PropertyChanges {
                            target:addStateBox
                            buttonColor:pressedColor
                        }
                    }
            ]
            
            transitions: [
                    Transition {
                        from: ""
                        to: "Hover"
                        reversible: true
                        ColorAnimation {
                            duration: 150
                            target: addStateBox
                            properties: "buttonColor"
                        }
                    }
                ]

            property variant buttonColor:"#282828"
            property variant defaultColor:"#282828"
            property variant hoverColor:"#E1E1E1"
            property variant pressedColor:Qt.darker("#282828")

            Rectangle {
                anchors.fill: parent
                color: "transparent"
                border.width: 1
                border.color: addStateBox.buttonColor
            }


            Rectangle {
                anchors.centerIn: parent
                width: 21
                height: width
                color:addStateBox.buttonColor
                radius: width/2
                id:plusSign


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
                    root.createNewState(); //create new state

                    // select the new state
                   // root.currentStateInternalId = statesEditorModel.count - 1;

                    // this should happen automatically
                    if (floatingNewStateBox.visible)
                    addStateBox.state = "Hover";
                }
                onPressed: addStateBox.state="Pressed"
                onReleased: addStateBox.state=""
                hoverEnabled: true
                onEntered: addStateBox.state="Hover"
                onExited: addStateBox.state=""

            }
        }
    }

    HorizontalScrollBar {
        id: horizontalScrollbar

        flickable: listView
        anchors.left: listView.left
        anchors.right : listView.right
        anchors.top : listView.bottom
        anchors.topMargin: 0
        anchors.rightMargin: 1
        anchors.leftMargin: 1
        height: 10
        onUnFocus: root.unFocus();
    }
}
