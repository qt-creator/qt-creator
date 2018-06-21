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

import QtQuick 2.2
import QtQuick.Controls 1.1
import QtQuick.Controls.Styles 1.1
import "../common"
import QtQuickDesignerTheme 1.0

FocusScope {
    id: root

    height: expanded ? 192 : 40
    signal createNewState
    signal deleteState(int internalNodeId)
    signal duplicateCurrentState

    property int stateImageSize: 180
    property int delegateWidth: stateImageSize + 44
    property int padding: 2
    property int delegateHeight: root.height - padding * 2 + 1
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
        color: Theme.qmlDesignerBackgroundColorDarkAlternate()
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

            onClicked: root.createNewState()

            style: ButtonStyle {
                background: Rectangle {
                    property color buttonBaseColor: Qt.darker(Theme.qmlDesignerBackgroundColorDarkAlternate(), 1.1)
                    color: control.hovered ? Qt.lighter(buttonBaseColor, 1.2)  : buttonBaseColor
                    border.color: Theme.qmlDesignerBorderColor()
                    border.width: 1
                    Image {
                        source: "image://icons/plus"
                        width: 16
                        height: 16
                        anchors.centerIn: parent
                        smooth: false
                    }
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
                baseColor: isCurrentState ? Theme.color(Theme.QmlDesigner_HighlightColor) : background.color
                delegateStateName: stateName
                delegateStateImageSource: stateImageSource
                delegateStateImageSize: stateImageSize
                delegateHasWhenCondition: hasWhenCondition
                delegateWhenConditionString: whenConditionString
            }
        }
    }
}
