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
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPLv3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
****************************************************************************/

import QtQuick 2.2
import QtQuick.Controls 1.1
import QtQuick.Controls.Styles 1.1
import "../common"

FocusScope {
    id: root

    height: expanded ? 136 : 32
    signal createNewState
    signal deleteState(int internalNodeId)
    signal duplicateCurrentState

    property int stateImageSize: 100
    property int delegateWidth: stateImageSize + 10
    property int padding: 2
    property int delegateHeight: root.height - padding * 2
    property int innerSpacing: -1
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

        property int buttonLeftSpacing: 0

        anchors.right: parent.right
        width: delegateHeight / 2 + buttonLeftSpacing
        height: delegateHeight

        Button {
            id: addStateButton
            visible: canAddNewStates

            tooltip: qsTr("Add a new state.")

            anchors.right: parent.right
            anchors.rightMargin: 8
            anchors.verticalCenter: parent.verticalCenter
            width: Math.max(parent.height / 2 - 8, 18)
            height: width
            iconSource: "images/plus.png"

            onClicked: root.createNewState()

            style: ButtonStyle {
                background: Rectangle {
                    property color buttonBaseColor: "#6f6f6f"
                    color: control.hovered ? Qt.lighter(buttonBaseColor, 1.2)  : buttonBaseColor
                    border.width: 1
                }
            }
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
                baseColor: isCurrentState ? Qt.darker(highlightColor, 1.2) : background.color
                delegateStateName: stateName
                delegateStateImageSource: stateImageSource
                delegateStateImageSize: stateImageSize
            }
        }
    }
}
