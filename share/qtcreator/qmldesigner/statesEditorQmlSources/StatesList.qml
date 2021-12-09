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
    property int effectiveHeight: root.expanded ? 287 : 85 // height of the states area

    signal createNewState
    signal deleteState(int internalNodeId)
    signal duplicateCurrentState

    property int stateImageSize: 200
    property int padding: 2
    property int delegateWidth: root.stateImageSize
                                + 2 * (root.delegateStateMargin + root.delegatePreviewMargin)
    property int delegateHeight: effectiveHeight
                                 - StudioTheme.Values.scrollBarThickness
                                 - 2 * (root.padding + StudioTheme.Values.border)
    property int innerSpacing: 2
    property int currentStateInternalId: 0

    property bool expanded: true

    Connections {
        target: statesEditorModel
        function onChangedToState(n) { root.currentStateInternalId = n }
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
                onTriggered: root.expanded = !root.expanded
            }
        }
    }

    AbstractButton {
        id: addStateButton

        buttonIcon: root.expanded ? qsTr("Create New State") : StudioTheme.Constants.plus
        iconFont: root.expanded ? StudioTheme.Constants.font : StudioTheme.Constants.iconFont
        iconSize: root.expanded ? StudioTheme.Values.myFontSize : StudioTheme.Values.myIconFontSize
        iconItalic: root.expanded
        tooltip: qsTr("Add a new state.")
        visible: canAddNewStates
        anchors.right: parent.right
        anchors.rightMargin: 8
        y: (Math.min(effectiveHeight, root.height) - height) / 2
        width: Math.max(root.delegateHeight / 2 - 8, 18)
        height: root.expanded ? 60 : width

        onClicked: {
            contextMenu.dismiss()
            root.createNewState()
        }
    }

    Rectangle { // separator lines between state items
        color: StudioTheme.Values.themeStateSeparator
        x: root.padding
        y: root.padding
        width: statesListView.width
        height: root.delegateHeight
    }

    ListView {
        id: statesListView

        boundsBehavior: Flickable.StopAtBounds
        clip: true

        x: root.padding
        y: root.padding
        width: Math.min(root.delegateWidth * statesListView.count + root.innerSpacing * (statesListView.count - 1),
                        root.width - addStateButton.width - root.padding - 16) // 16 = 2 * 8 (addStateButton margin)
        height: root.delegateHeight + StudioTheme.Values.scrollBarThickness

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
            onDelegateInteraction: contextMenu.dismiss()

            columnSpacing: root.delegateColumnSpacing
            topAreaHeight: root.delegateTopAreaHeight
            bottomAreaHeight: root.delegateBottomAreaHeight
            stateMargin: root.delegateStateMargin
            previewMargin: root.delegatePreviewMargin
        }

        ScrollBar.horizontal: HorizontalScrollBar {}
    }
}
