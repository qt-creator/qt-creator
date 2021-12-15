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

    readonly property int delegateTopAreaHeight: StudioTheme.Values.height + 8
    readonly property int delegateBottomAreaHeight: delegateHeight - 2 * delegateStateMargin - delegateTopAreaHeight - 2
    readonly property int delegateStateMargin: 16
    readonly property int delegatePreviewMargin: 10
    readonly property int effectiveHeight: root.height < 130 ? 89 : Math.min(root.height, 287)

    readonly property int scrollBarH: statesListView.ScrollBar.horizontal.scrollBarVisible ? StudioTheme.Values.scrollBarThickness : 0
    readonly property int listMargin: 10
    readonly property int delegateWidth: 264
    readonly property int delegateHeight: Math.max(effectiveHeight - scrollBarH - 2 * listMargin, 69)
    readonly property int innerSpacing: 2

    property int currentStateInternalId: 0

    signal createNewState
    signal deleteState(int internalNodeId)
    signal duplicateCurrentState

    Connections {
        target: statesEditorModel
        function onChangedToState(n) { root.currentStateInternalId = n }
    }

    Rectangle {
        id: background
        anchors.fill: parent
        color: StudioTheme.Values.themePanelBackground
    }

    AbstractButton {
        id: addStateButton

        buttonIcon: StudioTheme.Constants.plus
        iconFont: StudioTheme.Constants.iconFont
        iconSize: StudioTheme.Values.myIconFontSize
        tooltip: qsTr("Add a new state.")
        visible: canAddNewStates
        anchors.right: parent.right
        anchors.rightMargin: 4
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 4 + scrollBarH
        width: 30
        height: 30

        onClicked: root.createNewState()
    }

    ListView {
        id: statesListView

        clip: true
        anchors.fill: parent
        anchors.topMargin: listMargin
        anchors.leftMargin: listMargin
        anchors.rightMargin: listMargin

        model: statesEditorModel
        orientation: ListView.Horizontal
        spacing: root.innerSpacing

        delegate: StatesDelegate {
            id: statesDelegate
            width: root.delegateWidth
            height: root.delegateHeight
            isBaseState: 0 === internalNodeId
            isCurrentState: root.currentStateInternalId === internalNodeId
            delegateStateName: stateName
            delegateStateImageSource: stateImageSource
            delegateHasWhenCondition: hasWhenCondition
            delegateWhenConditionString: whenConditionString

            topAreaHeight: root.delegateTopAreaHeight
            bottomAreaHeight: root.delegateBottomAreaHeight
            stateMargin: root.delegateStateMargin
            previewMargin: root.delegatePreviewMargin
            scrollBarH: root.scrollBarH
            listMargin: root.listMargin
        }
        ScrollBar.horizontal: HorizontalScrollBar {}
    }
}
