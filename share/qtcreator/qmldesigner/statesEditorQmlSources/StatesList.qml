/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
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

import QtQuick 2.2
import QtQuick.Controls 1.1
import QtQuick.Controls.Styles 1.1
import "../common"

FocusScope {
    id: root

    height: expanded ? 132 : 32
    signal createNewState
    signal deleteState(int internalNodeId)
    signal duplicateCurrentState

    property int stateImageSize: 100
    property int delegateWidth: stateImageSize + 10
    property int padding: 2
    property int delegateHeight: root.height - padding * 2
    property int innerSpacing: 2
    property int currentStateInternalId : 0

    property bool expanded: true

    Connections {
        target: statesEditorModel
        onChangedToState: root.currentStateInternalId = n
    }

    SystemPalette {
        id: palette
    }

    Rectangle {
        id: background
        anchors.fill: parent
        color: "#4f4f4f"
    }

    MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.LeftButton | Qt.RightButton

        onClicked: {
            if (mouse.button === Qt.LeftButton)
                focus = true
            else if (mouse.button === Qt.RightButton)
                contextMenu.popup()
        }

        Menu {
            id: contextMenu

            MenuItem {
                text: root.expanded ? qsTr("Collapse") : qsTr("Expand")
                onTriggered: {
                    root.expanded = !root.expanded
                }

            }
        }
    }

    Item {
        id: addStateItem

        property int buttonLeftSpacing: innerSpacing

        anchors.right: parent.right
        width: delegateHeight / 2 + buttonLeftSpacing
        height: delegateHeight

        Button {
            style: ButtonStyle {
                background: Rectangle {
                    implicitWidth: 100
                    implicitHeight: 25
                    color: control.hovered ? "#6f6f6f" : "#4f4f4f"

                    border.color: "black"
                }
            }

            id: addStateButton
            visible: canAddNewStates

            anchors.right: parent.right
            anchors.rightMargin: padding
            anchors.verticalCenter: parent.verticalCenter
            width: Math.max(parent.height / 2, 24)
            height: width
            iconSource: "images/plus.png"

            onClicked: root.createNewState()
        }
    }

    ScrollView {
        anchors.left: parent.left
        anchors.right: addStateItem.left
        height: delegateHeight
        y: padding
        anchors.leftMargin: padding
        anchors.rightMargin: padding

        style: DesignerScrollViewStyle {
        }

        ListView {
            anchors.fill: parent
            model: statesEditorModel
            orientation: ListView.Horizontal
            spacing: innerSpacing

            delegate: StatesDelegate {
                id: statesDelegate
                width: delegateWidth
                height: delegateHeight
                isBaseState: 0 == internalNodeId
                isCurrentState: root.currentStateInternalId == internalNodeId
                gradiantBaseColor: isCurrentState ? Qt.darker(highlightColor, 1.2) : background.color
                delegateStateName: stateName
                delegateStateImageSource: stateImageSource
                delegateStateImageSize: stateImageSize
            }
            Rectangle {
                /* Rectangle at the bottom using the highlight color */
                anchors.bottom: parent.bottom
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.leftMargin: 1
                anchors.rightMargin: 1
                height: 4
                color: Qt.darker(highlightColor, 1.2)
            }
        }
    }
}
