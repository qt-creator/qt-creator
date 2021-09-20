/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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

    property int delegateTopAreaHeight: StudioTheme.Values.height + 8
    property int delegateBottomAreaHeight: 200
    property int delegateColumnSpacing: 2
    property int delegateStateMargin: 16
    property int delegatePreviewMargin: 16

    height: (root.expanded ? (root.delegateTopAreaHeight + root.delegateBottomAreaHeight + root.delegateColumnSpacing)
                           : root.delegateTopAreaHeight)
            + StudioTheme.Values.scrollBarThickness
            + 2 * (root.delegateStateMargin + StudioTheme.Values.border + root.padding)

    signal createNewState
    signal deleteState(int internalNodeId)
    signal duplicateCurrentState

    property int stateImageSize: 200
    property int padding: 2
    property int delegateWidth: root.stateImageSize
                                + 2 * (root.delegateStateMargin + root.delegatePreviewMargin)
    property int delegateHeight: root.height
                                 - StudioTheme.Values.scrollBarThickness
                                 - 2 * (root.padding + StudioTheme.Values.border)
    property int innerSpacing: 2
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
        color: StudioTheme.Values.themePanelBackground
    }

    MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.LeftButton | Qt.RightButton

        onClicked: function(mouse) {
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

        property int buttonLeftSpacing: 8 * (root.expanded ? 1 : 2)

        anchors.right: parent.right
        width: root.delegateHeight / 2 + buttonLeftSpacing
        height: root.delegateHeight

        AbstractButton {
            id: addStateButton

            buttonIcon: root.expanded ? qsTr("Create New State") : StudioTheme.Constants.plus
            iconFont: root.expanded ? StudioTheme.Constants.font : StudioTheme.Constants.iconFont
            iconSize: root.expanded ? StudioTheme.Values.myFontSize : StudioTheme.Values.myIconFontSize
            iconItalic: root.expanded ? true : false
            tooltip: qsTr("Add a new state.")
            visible: canAddNewStates
            anchors.right: parent.right
            anchors.rightMargin: 8
            anchors.verticalCenter: parent.verticalCenter
            width: Math.max(parent.height / 2 - 8, 18)
            height: root.expanded ? 80 : width

            onClicked: {
                root.closeContextMenu()
                root.createNewState()
            }
        }
    }

    Rectangle {
        color: StudioTheme.Values.themeStateSeparator
        x: root.padding
        y: root.padding
        width: Math.min((root.delegateWidth * flickable.count) + (2 * (flickable.count - 1)),
                        flickable.width)
        height: root.delegateHeight
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
            isBaseState: 0 === internalNodeId
            isCurrentState: root.currentStateInternalId === internalNodeId
            baseColor: isCurrentState ? StudioTheme.Values.themeInteraction : background.color
            delegateStateName: stateName
            delegateStateImageSource: stateImageSource
            delegateStateImageSize: stateImageSize
            delegateHasWhenCondition: hasWhenCondition
            delegateWhenConditionString: whenConditionString
            onDelegateInteraction: root.closeContextMenu()

            columnSpacing: root.delegateColumnSpacing
            topAreaHeight: root.delegateTopAreaHeight
            bottomAreaHeight: root.delegateBottomAreaHeight
            stateMargin: root.delegateStateMargin
            previewMargin: root.delegatePreviewMargin
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
