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

import QtQuick 1.1
import qtcomponents 1.0

Rectangle {
    id: delegate
    height: 240
    width: 216

    property alias caption: captionItem.text
    property alias imageSource: imageItem.source
    property alias videoSource: videoIcon.source
    property alias description: descriptionItem.text
    property bool isVideo: false
    property alias videoLength: length.text

    property alias tags: repeater.model

    function appendTag(tag) {
        var tagStr = "tag:" + '"' + tag + '"'
        if (searchBar.text === "")
            searchBar.text = tagStr
        else
            searchBar.text += " " + tagStr
    }

    CustomColors {
        id: colors
    }

    CustomFonts {
        id: fonts
    }

    QStyleItem { cursor: "pointinghandcursor"; anchors.fill: parent }


    BorderImage {
        id: image1
        x: 11
        y: 8
        width: 196
        height: 153
        anchors.horizontalCenter: parent.horizontalCenter
        border.bottom: 4
        border.right: 4
        border.top: 4
        border.left: 4
        source: "images/dropshadow.png"

        Image {
            id: imageItem

            visible: !delegate.isVideo
            anchors.centerIn: parent
            asynchronous: true
            sourceSize.height: 145
            sourceSize.width: 188
            fillMode: Image.Center
        }

        Image {
            id: videoIcon

            visible: delegate.isVideo
            anchors.centerIn: parent
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.verticalCenter: parent.verticalCenter
            asynchronous: true
        }

        Text {
            id: length

            visible: delegate.isVideo
            x: 87
            y: 130
            color: "#555555"
            text: delegate.videoLength
            anchors.horizontalCenter: parent.horizontalCenter
            font.bold: true
            font.family: "Helvetica"
            font.pixelSize: 14
        }
    }

    Rectangle {
        id: rectangle2
        y: 161
        width: 200
        height: 69
        color: "#ffffff"
        anchors.bottom: parent.bottom
        anchors.right: parent.right
        anchors.left: parent.left
    }

    Text {
        id: captionItem
        x: 16
        y: 170
        color: colors.strongForegroundColor
        text: qsTr("2D PAINTING EXAMPLE long description")
        clip: true
        anchors.right: parent.right
        anchors.rightMargin: 16
        anchors.left: parent.left
        anchors.leftMargin: 16
        wrapMode: Text.WordWrap
        maximumLineCount: 1
        font: fonts.standardCaption
    }

    Text {
        id: descriptionItem
        height: 43
        color: "#7e7e7e"
        text: qsTr("The 2D Painting example shows how QPainter and QGLWidget work together.")
        anchors.top: captionItem.bottom
        anchors.topMargin: 10
        opacity: 0
        anchors.left: parent.left
        anchors.leftMargin: 16
        anchors.right: parent.right
        anchors.rightMargin: 16
        wrapMode: Text.WordWrap
        font: fonts.standstandardDescription
        horizontalAlignment: Text.AlignJustify
        maximumLineCount: 8
    }

    Rectangle {
        id: rectangle1
        x: 16
        y: 195
        height: 1
        color: "#dddcdc"
        anchors.left: parent.left
        anchors.leftMargin: 10
        anchors.right: parent.right
        anchors.rightMargin: 10
    }

    Text {
        id: tags
        x: 16
        y: 198
        text: qsTr("Tags:")
        smooth: true
        font.italic: false
        font.pixelSize: 11
        wrapMode: Text.WordWrap
        font.family: "Helvetica"
        font.bold: false
    }


    Rectangle {
        id: rectangle3
        x: 10
        height: 1
        color: "#dddcdc"
        visible: false
        anchors.top: captionItem.bottom
        anchors.topMargin: 4
        anchors.rightMargin: 10
        anchors.right: parent.right
        anchors.leftMargin: 10
        anchors.left: parent.left
    }


    Rectangle {
        id: border
        color: "#00000000"
        radius: 8
        anchors.rightMargin: 4
        anchors.leftMargin: 4
        anchors.bottomMargin: 4
        anchors.topMargin: 4
        visible: false
        anchors.fill: parent
        border.color: "#dddcdc"
    }


    MouseArea {
        id: mousearea1
        anchors.fill: parent
        hoverEnabled: true
        onEntered: {
            delegate.state="hover"
        }

        onExited: {
            delegate.state=""
        }

        onClicked: {
            var oldPauseAnimation = pauseAnimation.duration
            pauseAnimation.duration = 10;
            delegate.state = ""
            pauseAnimation.duration = oldPauseAnimation;
            if (model.isVideo)
                gettingStarted.openUrl(model.videoUrl);
            else if (model.hasSourceCode)
                gettingStarted.openProject(model.projectPath, model.filesToOpen, model.docUrl, model.dependencies, model.platforms)
            else
                gettingStarted.openSplitHelp(model.docUrl);
        }

    }


    states: [
        State {
            name: "hover"

            PropertyChanges {
                target: rectangle2
                x: 0
                y: 4
                width: 216
                height: 236
            }

            PropertyChanges {
                target: captionItem
                y: 14
                maximumLineCount: 2
            }

            PropertyChanges {
                target: descriptionItem
                opacity: 1
            }

            PropertyChanges {
                target: rectangle3
                x: 10
                y: 52
                visible: true
                anchors.rightMargin: 10
                anchors.leftMargin: 10
            }

            PropertyChanges {
                target: border
                visible: true
            }
        }
    ]

    transitions: [
        Transition {
            from: ""
            to: "hover"
            SequentialAnimation {
                PauseAnimation { id: pauseAnimation; duration: 200  }
                ParallelAnimation {

                    PropertyAnimation {
                        properties: "y, height"
                        duration: 100
                    }
                    SequentialAnimation {
                        PropertyAction {
                            property: "maximumLineCount"
                        }

                        PauseAnimation { duration: 60 }
                        PropertyAnimation {
                            properties: "opacity"
                            duration: 150
                        }
                    }
                }
            }
        },
        Transition {
            from: "hover"
            to: ""
            SequentialAnimation {
                PauseAnimation { duration: 100  }

                ParallelAnimation {
                    PropertyAnimation {
                        properties: "opacity"
                        duration: 60
                    }
                    SequentialAnimation {
                        PauseAnimation { duration: 60 }
                        PropertyAction {
                            property: "maximumLineCount"
                        }
                        PropertyAnimation {
                            properties: "y, height"
                            duration: 100
                        }
                    }
                }
            }
        }
    ]
    Flow {
        y: 198
        width: 159
        height: 32
        anchors.left: tags.right
        anchors.leftMargin: 6
        clip: true

        spacing: 2

        Repeater {
            id: repeater
            model: mockupTags
            LinkedText {
                id: text4
                color: "#777777"
                text: modelData
                smooth: true
                font.pixelSize: 11
                height: 12
                font.family: "Helvetica" //setting the pixelSize will set the family back to the default
                wrapMode: Text.WordWrap
                onEntered: {
                    delegate.state="hover"
                }
                onExited: {
                    delegate.state=""
                }
                onClicked: appendTag(modelData)
                property bool hugeTag: (text.length > 12) && index > 1
                property bool isExampleTag: text === "example"
                visible: !hugeTag && !isExampleTag && index < 8
            }
        }
    }
    ListModel {
        id: mockupTags
        ListElement {
            modelData: "painting"
        }
        ListElement {
            modelData: "Qt Quick"
        }
        ListElement {
            modelData: "OpenGl"
        }

    }
}
