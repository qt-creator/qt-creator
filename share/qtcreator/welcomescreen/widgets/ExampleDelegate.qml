/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

import QtQuick 1.1

Item {
    id: delegate
    width: GridView.view.cellWidth
    height: GridView.view.cellHeight

    property variant tags : model.tags === undefined ? mockupTags : model.tags
    signal tagClicked(string tag)



    Rectangle {
        id: background
        radius: 3
        opacity: 0
        anchors.leftMargin: 4
        anchors.bottomMargin: 4
        anchors.topMargin: 4
        anchors.rightMargin: 4
        border.width: 0
        border.color: "#a8a8a8"
        anchors.fill: parent
    }

    Text {
        clip: true
        id: description
        color: "#292828"
        text: model.description
        anchors.top: image.bottom
        anchors.topMargin: 8
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 8
        anchors.right: parent.right
        anchors.rightMargin: 26
        anchors.left: parent.left
        anchors.leftMargin: 26

        wrapMode: TextEdit.WordWrap
        font.pixelSize: 13
        elide: Text.ElideRight
        textFormat: Text.PlainText
        maximumLineCount: 6

        Text {
            id: more
            x: 0
            color: "#70747c"
            text: "[...]"
            anchors.bottom: parent.bottom
            anchors.bottomMargin: -16
            font.pixelSize: 12
            font.bold: false
            visible: description.truncated
        }

    }

    MouseArea {
        anchors.fill: parent
        hoverEnabled: true
        onClicked: {
            delegate.state = "clicked"
        }
        onEntered: parent.state = "hover"
        onExited: if (parent.state === "hover") parent.state = ""
    }

    Rectangle {
        id: image
        x: 20
        y: 52
        anchors.top: parent.top
        anchors.topMargin: 52
        height: 232
        radius: 3
        smooth: true
        anchors.right: parent.right
        anchors.rightMargin: 20
        anchors.left: parent.left
        anchors.leftMargin: 20
        gradient: Gradient {
            GradientStop {
                position: 0
                color: "#e6e6e6"
            }

            GradientStop {
                position: 1
                color: "#e4e4e4"
            }
        }
        border.color: "#b5b1b1"

        Rectangle {
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.topMargin: 1
            anchors.leftMargin: 2
            anchors.rightMargin: 2
            height: 1
        }



        Image {
            property bool hideImage : model.imageUrl === "" || status === Image.Error
            fillMode: Image.PreserveAspectFit
            property string mockupSource: model.imageSource
            property string realSource: model.imageUrl !== "" ? "image://helpimage/" + encodeURI(model.imageUrl) : ""

            source: mockupSource === "" ? realSource : mockupSource
            asynchronous: true
            smooth: true

            anchors.top: parent.top
            anchors.topMargin: 46
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 16
            anchors.left: parent.left
            anchors.leftMargin: 16
            anchors.right: parent.right
            anchors.rightMargin: 16
        }

        Rectangle {
            id: tagLine;
            radius: 3
            anchors.top: parent.top
            anchors.topMargin: 10
            smooth: true
            height: 32
            color: "#00000000"
            anchors.right: parent.right
            anchors.rightMargin: 8
            anchors.left: parent.left
            anchors.leftMargin: 8

            Flow {
                x: 38
                y: 0
                width: 256
                height: 32
                anchors.left: text1.right
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                anchors.top: parent.top
                anchors.leftMargin: 4
                spacing: 2
                Repeater {
                    id: repeater
                    model: tags
                    Text {
                        states: [ State { name: "hover"; PropertyChanges { target: tagText; color: "black" } } ]
                        transitions: [
                            Transition {
                                PropertyAnimation { target: tagText; property: "color"; duration: 80 }
                            }
                        ]
                        id: tagText
                        property string delimiter: index === (repeater.count - 1) ? "" : ", "
                        text: model.modelData + delimiter
                        font.italic: true
                        color: "#a6a6a6"
                        font.pixelSize: 12
                        smooth: true
                        MouseArea {
                            anchors.fill: parent;
                            hoverEnabled: true;
                            onEntered: {
                                delegate.state = "hover"
                                parent.state = "hover"
                            }
                            onExited:{
                                delegate.state = ""
                                parent.state = ""
                            }
                            onClicked: {
                                delegate.tagClicked(model.modelData)
                            }
                        }
                    }
                }

            }

            Text { id: text1; x: 0; y: 0; text: qsTr("Tags:") ; font.bold: true; font.italic: true; color: "#8b8b8b"; font.pixelSize: 12}
        }
    }




    Caption {
        text: model.name
        id: caption
        y: 16
    }

//    Rectangle {
//        anchors.top: parent.top
//        height: 14
//        anchors.left: parent.left
//        anchors.right: parent.right
//        gradient: Gradient {
//            GradientStop {
//                position: 0.00;
//                color: "#ffffff";
//            }
//            GradientStop {
//                position: 1.00;
//                color: "#00ffffff";
//            }
//        }
//    }



    states: [
        State {
            name: "hover";

            PropertyChanges {
                target: background
                color: "#dddddd"
                opacity: 1
                border.width: 1
            }

        },
        State {
            name: "clicked"
            PropertyChanges {
                target: background
                color: "#e8e8e8"
                opacity: 1
                border.width: 1
            }
        }
    ]

    transitions: [
        Transition {
            from: ""
            to: "hover"
            reversible: true
            PropertyAnimation { target: background; properties: "opacity, color"; duration: 60 }
        },
        Transition {
            from: "clicked"
            to: ""
            PropertyAnimation { target: background; properties: "opacity, color"; duration: 60 }
        },
        Transition {
            from: "hover"
            to: "clicked"
            SequentialAnimation {
                PropertyAnimation { target: background; properties: "opacity, color"; duration: 60 }
                ScriptAction {
                    script: {
                        delegate.state = "";
                        if (model.hasSourceCode)
                            gettingStarted.openProject(model.projectPath, model.filesToOpen, model.docUrl)
                        else
                            gettingStarted.openSplitHelp(model.docUrl);
                    }
                }
            }
        }
    ]
    //    Rectangle {
    //        id: line
    //        height: 6
    //        color: "#eaeaea"
    //        anchors.rightMargin: -20
    //        anchors.leftMargin: -20
    //        anchors.bottom: parent.bottom
    //        anchors.bottomMargin: -4
    //        anchors.left: parent.left
    //        anchors.right: parent.right
    //    }
}
