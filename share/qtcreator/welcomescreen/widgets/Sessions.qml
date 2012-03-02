/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
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

import QtQuick 1.0
import qtcomponents 1.0

Item {
    property alias model: root.model
    height: Math.min(root.count * root.delegateHeight, 276)


    ListView {
        id: root

        anchors.fill: parent

        snapMode: ListView.SnapToItem
        property int delegateHeight: currentItem.height + spacing

        clip: true
        interactive: false

        spacing: 4

        property bool delayedHide: false

        Behavior on delayedHide {
            PropertyAnimation { duration: 200; }
        }

        onDelayedHideChanged: {
            panel.opacity = 0;
        }

        delegate: SessionItem {
            id: item

            property bool activate: hovered

            Behavior on activate {
                PropertyAnimation { duration: 50 }
            }

            onActivateChanged: {
                if (activate) {
                    panel.y = item.y + 20 - root.contentY;
                    panel.opacity = 1;
                    panel.projectsPathList = projectsPath;
                    panel.projectsDisplayList = projectsName
                    panel.currentSession = sessionName;
                } else {
                    if (!panel.hovered)
                        panel.opacity = 0
                }
            }

            onClicked: {
                root.delayedHide = !root.delayedHide
            }

            function fullSessionName()
            {
                var newSessionName = sessionName
                if (model.lastSession && sessionList.isDefaultVirgin())
                    newSessionName = qsTr("%1 (last session)").arg(sessionName);
                else if (model.activeSession && !sessionList.isDefaultVirgin())
                    newSessionName = qsTr("%1 (current session)").arg(sessionName);
                return newSessionName;
            }

            name: fullSessionName()
        }

        WheelArea {
            id: wheelarea
            anchors.fill: parent
            verticalMinimumValue: vscrollbar.minimumValue
            verticalMaximumValue: vscrollbar.maximumValue

            onVerticalValueChanged: root.contentY =  Math.round(verticalValue / root.delegateHeight) * root.delegateHeight
        }

        ScrollBar {
            id: vscrollbar
            orientation: Qt.Vertical
            property int availableHeight : root.height
            visible: root.contentHeight > availableHeight
            maximumValue: root.contentHeight > availableHeight ? root.contentHeight - availableHeight : 0
            minimumValue: 0
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            singleStep: root.delegateHeight
            anchors.topMargin: styleitem.style == "mac" ? 1 : 0
            onValueChanged: root.contentY =  Math.round(value / root.delegateHeight) * root.delegateHeight
            anchors.rightMargin: styleitem.frameoffset
            anchors.bottomMargin: styleitem.frameoffset
        }
    }

    Rectangle {

        CustomFonts {
            id: fonts
        }

        id: panel

        border.width: 1
        smooth: true
        opacity: 0

        property int margin: 12

        width: panelColumn.width + margin * 2
        height: panelColumn.height + margin * 2

        property bool hovered: false

        property variant projectsPathList
        property variant projectsDisplayList

        property string currentSession

        onHoveredChanged: {
            if (panel.hovered)
                panel.opacity = 1;
            else
                panel.opacity = 0;
        }

        MouseArea {
            anchors.topMargin: 0
            anchors.fill: parent
            id: panelMouseArea
            hoverEnabled: true
            onEntered: {
                panel.hovered = true
            }
            onExited: {
                panel.hovered = false
            }
        }

        Column {
            x: panel.margin
            y: panel.margin
            id: panelColumn
            spacing: 8

            Repeater {
                model: panel.projectsPathList
                delegate: Row {
                    spacing: 6
                    Text {
                        text: panel.projectsDisplayList[index]
                        font: fonts.boldDescription
                    }
                    Text {
                        text: modelData
                        font: fonts.linkFont
                    }
                }
            }

            Row {
                x: -2
                spacing: 2
                id: add

                Item {
                    //place hold for an icon
                    width: 16
                    height: 16

                    MouseArea {
                        id: exitMouseArea
                        anchors.fill: parent
                        hoverEnabled: true
                        onEntered: {
                            panel.hovered = true
                        }
                        onExited: {
                            panel.hovered = false
                        }
                        onClicked: {
                            //Will be uncommented once we have an icon
                            //model.cloneSession(panel.currentSession);
                        }
                    }
                }
                LinkedText {
                    text: qsTr("Clone this session")
                    onEntered: {
                        panel.hovered = true
                    }
                    onExited: {
                        panel.hovered = false
                    }
                    onClicked: {
                        panel.opacity = 0;
                        model.cloneSession(panel.currentSession);
                    }
                }
            }
            Row {
                x: -2
                spacing: 2
                id: clear

                Item {
                    //place holder for an icon
                    width: 16
                    height: 16


                    MouseArea {
                        id: clearMouseArea
                        anchors.fill: parent
                        hoverEnabled: true
                        onEntered: {
                            panel.hovered = true
                        }
                        onExited: {
                            panel.hovered = false
                        }
                        onClicked: {
                            //Will be uncommented once we have an icon
                            //model.deleteSession(panel.currentSession);
                        }
                    }
                }
                LinkedText {
                    text: qsTr("Delete this session")
                    onEntered: {
                        panel.hovered = true
                    }
                    onExited: {
                        panel.hovered = false
                    }
                    onClicked: {
                        panel.opacity = 0;
                        model.deleteSession(panel.currentSession);
                    }
                }
            }
            Row {
                x: -2
                spacing: 2
                id: rename

                Item {
                    //place holder for an icon
                    opacity: clearMouseArea.containsMouse ? 0.8 : 1
                    Behavior on opacity {
                        PropertyAnimation { duration: visible ? 0 : 50; }
                    }

                    width: 16
                    height: 16

                    MouseArea {
                        id: renameMouseArea
                        anchors.fill: parent
                        hoverEnabled: true
                        onEntered: {
                            panel.hovered = true
                        }
                        onExited: {
                            panel.hovered = false
                        }
                        onClicked: {
                            //Will be uncommented once we have an icon
                            //model.renameSession(panel.currentSession);
                        }
                    }
                }
                LinkedText {
                    text: qsTr("Rename this session")
                    onEntered: {
                        panel.hovered = true
                    }
                    onExited: {
                        panel.hovered = false
                    }
                    onClicked: {
                        panel.opacity = 0;
                        model.renameSession(panel.currentSession);
                    }
                }
            }
        }

        Behavior on opacity {
            PropertyAnimation { duration: visible ? 50 : 100; }
        }
        radius: 2
        gradient: Gradient {
            GradientStop {
                position: 0.00;
                color: "#ffffff";
            }
            GradientStop {
                position: 1.00;
                color: "#e4e5f0";
            }
        }
    }
}
