/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

import QtQuick 2.1

Item {
    id: delegate
    property bool expanded: false
    height: columns.height
    width: columns.width
    property alias name: titleText.text

    Column {
        id: columns

        Rectangle {
            id: rectangle
            height: 30
            width: 260

            color: (titleArea.containsMouse || collapseArea.containsMouse || delegate.expanded)
                   ? creatorTheme.Welcome_HoverColor
                   : creatorTheme.Welcome_BackgroundColor

            Image {
                id: sessionIcon
                source: "image://icons/session/Welcome_ForegroundSecondaryColor"
                x: 11
                anchors.verticalCenter: parent.verticalCenter
                height: 16
                width: 16
            }

            NativeText {
                id: titleText
                anchors.fill: parent
                anchors.leftMargin: 38
                elide: Text.ElideRight
                color: creatorTheme.Welcome_LinkColor
                verticalAlignment: Text.AlignVCenter
                font.pixelSize: fonts.linkFont.pixelSize
                font.family: fonts.linkFont.family
                font.underline: titleArea.containsMouse
            }

            MouseArea {
                id: titleArea
                hoverEnabled: true
                anchors.fill: parent
                onClicked: {
                    projectWelcomePage.requestSession(sessionName);
                }
            }
        }
        Rectangle {
            z: -1
            property int margin: 6
            id: details
            height: expanded ? innerColumn.height + margin + 16 : 0
            width: titleArea.width + collapseArea.width
            color: creatorTheme.Welcome_HoverColor
            clip: true
            visible: false

            Behavior on height {
                SequentialAnimation {
                    ScriptAction {
                        script: if (expanded) details.visible = true;
                    }
                    NumberAnimation {
                        duration: 180
                        easing.type: Easing.InOutQuad
                    }
                    ScriptAction {
                        script: if (!expanded) details.visible = false;
                    }
                }
            }

            Column {
                x: titleText.x
                y: parent.margin
                id: innerColumn
                spacing: 12

                Repeater {
                    model: projectsPath
                    delegate: Column {
                        spacing: 4
                        NativeText {
                            text: projectsName[index]
                            font: fonts.smallPath
                            color: creatorTheme.Welcome_TextColor
                            width: titleText.width
                        }
                        NativeText {
                            text: modelData
                            font: fonts.smallPath
                            elide: Text.ElideRight
                            color: creatorTheme.Welcome_ForegroundPrimaryColor
                            width: titleText.width
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
                    width: parent.width - 2 * parent.margin
                    height: 18
                    spacing: 6

                    SessionActionLabel {
                        text: qsTr("Clone")
                        onClicked: root.model.cloneSession(sessionName)
                    }

                    Rectangle {
                        visible: !defaultSession
                        width: 1;
                        height: 13;
                        color: creatorTheme.Welcome_ForegroundSecondaryColor
                    }

                    SessionActionLabel {
                        visible: !defaultSession
                        text: qsTr("Rename")
                        onClicked: root.model.renameSession(sessionName)
                    }

                    Rectangle {
                        visible: y === 0 && !defaultSession
                        width: 1;
                        height: 13;
                        color: creatorTheme.Welcome_ForegroundSecondaryColor
                    }

                    SessionActionLabel {
                        visible: !defaultSession
                        text: qsTr("Delete")
                        onClicked: root.model.deleteSession(sessionName)
                    }
                }
            }
        }
    }

    Item {
        x: rectangle.width
        width: 28
        height: titleArea.height
        Rectangle {
            id: collapseButton
            anchors.fill: parent
            color: (collapseArea.containsMouse || delegate.expanded)
                   ? creatorTheme.Welcome_HoverColor
                   : creatorTheme.Welcome_BackgroundColor

            Image {
                x: 6
                y: 7
                visible: (collapseArea.containsMouse || delegate.expanded || titleArea.containsMouse)
                source: "image://icons/expandarrow/Welcome_ForegroundSecondaryColor"
                rotation: delegate.expanded ? 180 : 0
                height: 16
                width: 16
            }
        }

        MouseArea {
            id: collapseArea
            anchors.fill: parent
            hoverEnabled: true
            onClicked: {
                delegate.expanded = !delegate.expanded;
                delegate.ListView.view.positionViewAtIndex(index, ListView.Contain);
            }
        }
    }
}
