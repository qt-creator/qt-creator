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

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuickDesignerTheme 1.0
import HelperWidgets 2.0
import StudioControls 1.0 as StudioControls
import StudioTheme 1.0 as StudioTheme

FocusScope {
    id: root

    height: (root.expanded ? 192 : 40) + StudioTheme.Values.scrollBarThickness
    signal createNewState
    signal deleteState(int internalNodeId)
    signal duplicateCurrentState

    property int stateImageSize: 180
    property int padding: 2
    property int delegateWidth: root.stateImageSize + 44
    property int delegateHeight: root.height - StudioTheme.Values.scrollBarThickness - root.padding * 2 + 1
    property int innerSpacing: 0
    property int currentStateInternalId: 0

    property bool expanded: true

    Connections {
        target: statesEditorModel
        function onChangedToState(n) { root.currentStateInternalId = n }
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
            if (mouse.button === Qt.LeftButton) {
                contextMenu.dismiss()
                focus = true
            } else if (mouse.button === Qt.RightButton) {
                contextMenu.popup()
            }
        }

        StudioControls.Menu {
            id: contextMenu

            StudioControls.MenuItem {
                text: root.expanded ? qsTr("Collapse") : qsTr("Expand")
                onTriggered: {
                    root.expanded = !root.expanded
                }
            }
        }
    }

    function closeContextMenu() {
        if (contextMenu.open)
            contextMenu.dismiss()
    }

    Item {
        id: addStateItem

        property int buttonLeftSpacing: 8 * (root.expanded ?  1 : 2)

        anchors.right: parent.right
        width: root.delegateHeight / 2 + buttonLeftSpacing
        height: root.delegateHeight

        AbstractButton {
            id: addStateButton
            visible: canAddNewStates

            tooltip: qsTr("Add a new state.")

            anchors.right: parent.right
            anchors.rightMargin: 8
            anchors.verticalCenter: parent.verticalCenter
            width: Math.max(parent.height / 2 - 8, 18)
            height: width

            onClicked: {
                root.closeContextMenu()
                root.createNewState()
            }

            background: Rectangle {
                property color buttonBaseColor: Qt.darker(Theme.qmlDesignerBackgroundColorDarkAlternate(), 1.1)
                color: addStateButton.hovered ? Qt.lighter(buttonBaseColor, 1.2) : buttonBaseColor
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

    ListView {
        id: flickable

        boundsBehavior: Flickable.StopAtBounds
        clip: true

        anchors.left: parent.left
        anchors.right: addStateItem.left
        height: root.delegateHeight + StudioTheme.Values.scrollBarThickness
        y: root.padding
        anchors.leftMargin: root.padding
        anchors.rightMargin: root.padding

        model: statesEditorModel
        orientation: ListView.Horizontal
        spacing: root.innerSpacing

        delegate: StatesDelegate {
            id: statesDelegate
            width: root.delegateWidth
            height: root.delegateHeight
            isBaseState: 0 == internalNodeId
            isCurrentState: root.currentStateInternalId == internalNodeId
            baseColor: isCurrentState ? Theme.color(Theme.QmlDesigner_HighlightColor) : background.color
            delegateStateName: stateName
            delegateStateImageSource: stateImageSource
            delegateStateImageSize: stateImageSize
            delegateHasWhenCondition: hasWhenCondition
            delegateWhenConditionString: whenConditionString
            onDelegateInteraction: root.closeContextMenu()
        }

        property bool bothVisible: horizontal.scrollBarVisible && vertical.scrollBarVisible

        ScrollBar.horizontal: HorizontalScrollBar {
            id: horizontal
            parent: flickable
        }

        ScrollBar.vertical: VerticalScrollBar {
            id: vertical
            parent: flickable
        }
    }
}
