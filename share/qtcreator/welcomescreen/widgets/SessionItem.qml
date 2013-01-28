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

Item {
    x: 5
    id: delegate
    property bool expanded: false
    height: column.height
    property alias name: text.text

    CustomFonts {
        id: fonts
    }

    Column {
        id: column
        spacing: 4

        Row {
            id: row1
            height: text.height + 8

            spacing: 4

            Image {
                anchors.verticalCenter: text.verticalCenter
                source: "images/bullet.png"
            }

            LinkedText {
                id: text

                onClicked: {
                    projectWelcomePage.requestSession(sessionName);
                }

                width: delegate.ListView.view.width - 80
                elide: Text.ElideRight

                enlargeMouseArea: false

                Rectangle {
                    z: -4
                    color: "#f9f9f9"
                    anchors.fill: parent
                    anchors.rightMargin: - delegate.ListView.view.width + text.width + 32
                    anchors.topMargin: -4
                    anchors.bottomMargin: -4
                    opacity: iArea.hovered || text.hovered || area2.hovered ? 1: 0.1
                    MouseArea {
                        acceptedButtons: Qt.RightButton
                        id: area2
                        hoverEnabled: true
                        anchors.fill: parent
                        property bool hovered: false
                        onEntered: {
                            area2.hovered = true
                        }
                        onExited: {
                            area2.hovered = false
                        }
                        onClicked: {
                            delegate.expanded = !delegate.expanded;
                            delegate.ListView.view.positionViewAtIndex(index, ListView.Contain);
                            area2.hovered = false
                        }
                    }
                }
            }

        }
        Item {
            z: -1
            property int margin: 6
            height: innerColumn.height + margin * 2
            width: delegate.ListView.view.width - 8 - margin * 2
            opacity: delegate.expanded ? 1 : 0

            Behavior on opacity {
                ParallelAnimation {
                    PauseAnimation { duration: delegate.expanded ? 100 : 0; }
                    PropertyAnimation { duration:  100; }
                }
            }

            Column {
                y: -4
                x: parent.margin + 8
                //y: parent.margin
                id: innerColumn
                spacing: 12
                width: parent.width - 16

                Repeater {
                    model: projectsPath
                    delegate: Column {
                        Text {
                            text: projectsName[index]
                            font: fonts.boldDescription
                        }
                        Text {
                            x: 4
                            function multiLinePath(path) {
                                if (path.length < 42)
                                    return path;
                                var index = 0;
                                var oldIndex = 0;
                                while (index != -1 && index < 40) {
                                    oldIndex = index;
                                    index = path.indexOf("/", oldIndex + 1);
                                    if (index == -1)
                                        index = path.indexOf("\\", oldIndex + 1);
                                }
                                var newPath = path.substr(0, oldIndex + 1) + "\n"
                                        + path.substr(oldIndex + 1, path.length - oldIndex - 1);
                                return newPath;
                            }
                            text: multiLinePath(modelData)
                            font: fonts.smallPath
                            wrapMode: Text.WrapAnywhere
                            maximumLineCount: 2
                            elide: Text.ElideRight
                            height: lineCount == 2 ? font.pixelSize * 2 + 4 : font.pixelSize + 2
                            color: "#6b6b6b"
                            width: delegate.ListView.view.width - 48
                            MouseArea {
                                anchors.fill: parent
                                hoverEnabled: true
                                onEntered: {
                                    toolTip.show();
                                }
                                onExited: {
                                    toolTip.hide()
                                }

                            }
                            ToolTip {
                                x: 10
                                y: 20
                                id: toolTip
                                text: modelData
                            }
                        }
                    }
                }

                Flow {
                    x: 6
                    width: parent.width -12
                    spacing: 4
                    visible: !defaultSession

                    Image { source: "images/icons/clone.png" }
                    LinkedText {
                        text: qsTr("Clone")
                        onClicked: {
                            root.model.cloneSession(sessionName);
                        }
                    }

                    Text { width: 16; text: " "; }
                    Image { source: "images/icons/rename.png" }
                    LinkedText {
                        text: qsTr("Rename")
                        onClicked: {
                            root.model.renameSession(sessionName);
                        }
                    }

                    Text { width: 16; text: " "; visible: y === 0}
                    Image { source: "images/icons/delete.png" }
                    LinkedText {
                        text: qsTr("Delete")
                        onClicked: {
                            root.model.deleteSession(sessionName);
                        }
                    }

                }
            }
            Rectangle {
                color: "#f1f1f1"
                radius: 4
                anchors.fill: parent
                anchors.topMargin: -8
                anchors.bottomMargin: -2
                anchors.leftMargin: 6
                anchors.rightMargin: 6
                z: -1
            }

        }
    }

    Item {
        x: delegate.ListView.view.width - 65
        width: 38
        height: 20
        Item {
            id: collapseButton
            opacity: text.hovered || area2.hovered || iArea.hovered || delegate.expanded ? 1 : 0

            property color color: iArea.hovered ? "#E9E9E9" : "#f1f1f1"

            anchors.fill: parent
            Image {
                x: 4
                source: "images/info.png"
            }
            Image {
                x: 20
                source: delegate.expanded ? "images/arrow_up.png" : "images/arrow_down.png"
            }
            Rectangle {
                color: collapseButton.color
                z: -1
                radius: 6
                anchors.fill: parent
                anchors.topMargin: -3
                anchors.bottomMargin: 6
                anchors.leftMargin: -1
                anchors.rightMargin: -1
                visible: iArea.hovered || delegate.expanded
            }

            Rectangle {
                color: collapseButton.color
                z: -1
                anchors.fill: parent
                anchors.topMargin: 6
                anchors.bottomMargin: delegate.expanded ? -2 : 2
                anchors.leftMargin: -1
                anchors.rightMargin: -1
                visible: iArea.hovered || delegate.expanded
            }
        }

        MouseArea {
            id: iArea
            anchors.margins: -2
            anchors.fill: parent
            hoverEnabled: true
            property bool hovered: false

            onClicked: {
                delegate.expanded = !delegate.expanded;
                delegate.ListView.view.positionViewAtIndex(index, ListView.Contain);
            }
            onEntered: {
                iArea.hovered = true
            }
            onExited: {
                iArea.hovered = false
            }
        }

    }
}
