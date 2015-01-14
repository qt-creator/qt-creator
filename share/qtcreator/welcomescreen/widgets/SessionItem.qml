/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

import QtQuick 2.1

Item {
    x: 5
    id: delegate
    property bool expanded: false
    height: columns.height
    width: columns.width
    property alias name: text.text

    Column {
        id: columns

        Row {
            id: row1
            height: text.height

            spacing: 7

            Image {
                source: "images/sessions.png"
                anchors.verticalCenter: text.verticalCenter
                width: 16
                height: 16
            }

            LinkedText {
                id: text

                onClicked: projectWelcomePage.requestSession(sessionName);

                width: delegate.ListView.view.width - 80
                height: 28
                elide: Text.ElideRight

                enlargeMouseArea: false

                Rectangle {
                    z: -4
                    // background of session item
                    color: creatorTheme.Welcome_SessionItem_BackgroundColorHover
                    anchors.fill: parent
                    visible: iArea.containsMouse || text.hovered
                    anchors.topMargin: 1
                    anchors.bottomMargin: 1
                    anchors.leftMargin: -row1.spacing / 2
                }
            }
        }
        Rectangle {
            z: -1
            property int margin: 6
            id: details
            height: expanded ? innerColumn.height + margin * 2 : 0
            width: delegate.ListView.view.width - 8 - margin * 2
            color: "#f1f1f1"
            radius: 4
            clip: true
            visible: false

            Behavior on height {
                SequentialAnimation {
                    ScriptAction {
                        script: if (expanded) details.visible = true;
                    }
                    NumberAnimation {
                        duration: 200
                        easing.type: Easing.InOutQuad
                    }
                    ScriptAction {
                        script: if (!expanded) details.visible = false;
                    }
                }
            }

            Column {
                x: parent.margin + 8
                y: parent.margin
                id: innerColumn
                spacing: 12
                width: parent.width - 16

                Repeater {
                    model: projectsPath
                    delegate: Column {
                        NativeText {
                            text: projectsName[index]
                            font: fonts.boldDescription
                            color: creatorTheme.Welcome_TextColorNormal
                        }
                        NativeText {
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
                            color: creatorTheme.Welcome_ProjectItem_TextColorFilepath
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
                    x: parent.margin
                    width: parent.width - 2 * parent.margin
                    height: 18
                    spacing: 4

                    Image { source: "images/icons/clone.png" }
                    LinkedText {
                        text: qsTr("Clone")
                        onClicked: {
                            root.model.cloneSession(sessionName);
                        }
                    }

                    Item {
                        visible: !defaultSession
                        width: 16;
                        height: 10;
                    }
                    Image {
                        visible: !defaultSession
                        source: "images/icons/rename.png"
                    }
                    LinkedText {
                        visible: !defaultSession
                        text: qsTr("Rename")
                        onClicked: {
                            root.model.renameSession(sessionName);
                        }
                    }

                    Item {
                        visible: y === 0 && !defaultSession
                        width: 16;
                        height: 10;
                    }
                    Image {
                        visible: !defaultSession
                        source: "images/icons/delete.png"
                    }
                    LinkedText {
                        visible: !defaultSession
                        text: qsTr("Delete")
                        onClicked: {
                            root.model.deleteSession(sessionName);
                        }
                    }
                }
            }
        }
    }

    Item {
        x: delegate.ListView.view.width - 65
        width: 38
        height: text.height
        Item {
            id: collapseButton
            visible: text.hovered || iArea.containsMouse || delegate.expanded

            property color color: iArea.containsMouse ? creatorTheme.Welcome_SessionItemExpanded_BackgroundColorHover
                                                      : creatorTheme.Welcome_SessionItemExpanded_BackgroundColorNormal

            anchors.fill: parent
            Image {
                x: 4
                y: 7
                source: "images/info.png"
            }
            Image {
                x: 20
                y: 7
                source: delegate.expanded ? "images/arrow_up.png" : "images/arrow_down.png"
            }
            Rectangle {
                color: collapseButton.color
                z: -1
                radius: creatorTheme.WidgetStyle === 'StyleFlat' ? 0 : 6
                anchors.fill: parent
                anchors.topMargin: 1
                anchors.bottomMargin: 1
            }

            Rectangle {
                color: collapseButton.color
                z: -1
                anchors.fill: parent
                anchors.topMargin: 6
                visible: details.visible
            }
        }

        MouseArea {
            id: iArea
            anchors.fill: parent
            hoverEnabled: true
            onClicked: {
                delegate.expanded = !delegate.expanded;
                delegate.ListView.view.positionViewAtIndex(index, ListView.Contain);
            }
        }
    }
}
