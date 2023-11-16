/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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

import QtQuick
import QtQuick.Controls.Basic as Basic
import StatesEditor
import HelperWidgets 2.0 as HelperWidgets
import StudioControls 1.0 as StudioControls
import StudioTheme as StudioTheme
import StatesEditorBackend

Rectangle {
    id: root

    signal createNewState
    signal cloneState(int internalNodeId)
    signal extendState(int internalNodeId)
    signal deleteState(int internalNodeId)

    property bool isLandscape: true

    color: StudioTheme.Values.themePanelBackground

    onWidthChanged: root.responsiveResize(root.width, root.height)
    onHeightChanged: root.responsiveResize(root.width, root.height)

    function showEvent() {
        addCanvas.requestPaint()
    }

    Component.onCompleted: root.responsiveResize(root.width, root.height)

    function numFit(overall, size, space) {
        let tmpNum = Math.floor(overall / size)
        let spaceLeft = overall - (tmpNum * size)
        return spaceLeft - (space * (tmpNum - 1)) >= 0 ? tmpNum : tmpNum - 1
    }

    function responsiveResize(width, height) {
        height -= toolBar.height + (2 * root.padding)
        width -= (2 * root.padding)

        var numStates = statesRepeater.count - 1 // Subtract base state
        var numRows = 0
        var numColumns = 0

        // Size extension in case of extend groups are shown
        var sizeExtension = root.showExtendGroups ? root.extend : 0
        var doubleSizeExtension = root.showExtendGroups ? 2 * root.extend : 0

        // Get view orientation (LANDSCAPE, PORTRAIT)
        if (width >= height) {
            root.isLandscape = true
            outerGrid.columns = 3
            outerGrid.rows = 1
            // Three outer section height (base state, middle, plus button)
            baseStateWrapper.height = height
            root.scrollViewHeight = height
            addWrapper.height = height

            height -= doubleSizeExtension

            if (height > Constants.maxThumbSize) {
                // In this case we want to have a multi row grid in the center
                root.thumbSize = Constants.maxThumbSize

                let tmpScrollViewWidth = width - root.thumbSize * 1.5 - 2 * root.outerGridSpacing

                // Inner grid calculation
                numRows = root.numFit(height, Constants.maxThumbSize, root.innerGridSpacing)
                numColumns = Math.min(numStates, root.numFit(tmpScrollViewWidth, root.thumbSize,
                                                             root.innerGridSpacing))

                let tmpRows = Math.ceil(numStates / numColumns)

                if (tmpRows <= numRows)
                    numRows = tmpRows
                else
                    numColumns = Math.ceil(numStates / numRows)
            } else {
                // This case is for single row layout and small thumb view
                root.thumbSize = Math.max(height, Constants.minThumbSize)

                // Inner grid calculation
                numColumns = numStates
                numRows = 1
            }

            Constants.thumbnailSize = root.thumbSize

            let tmpWidth = root.thumbSize * numColumns + root.innerGridSpacing * (numColumns - 1) + doubleSizeExtension
            let remainingSpace = width - root.thumbSize - 2 * root.outerGridSpacing
            let space = remainingSpace - tmpWidth

            if (space >= root.thumbSize) {
                root.scrollViewWidth = tmpWidth
                addWrapper.width = space
            } else {
                addWrapper.width = Math.max(space, 0.5 * root.thumbSize)
                root.scrollViewWidth = remainingSpace - addWrapper.width
            }

            root.topMargin = (root.scrollViewHeight - (root.thumbSize * numRows)
                              - root.innerGridSpacing * (numRows - 1)) * 0.5 - sizeExtension

            addCanvas.width = Math.min(addWrapper.width, root.thumbSize)
            addCanvas.height = root.thumbSize

            baseStateWrapper.width = root.thumbSize

            baseStateThumbnail.anchors.verticalCenter = baseStateWrapper.verticalCenter
            baseStateThumbnail.anchors.horizontalCenter = undefined

            addCanvas.anchors.verticalCenter = addWrapper.verticalCenter
            addCanvas.anchors.horizontalCenter = undefined
            addCanvas.anchors.top = undefined
            addCanvas.anchors.left = addWrapper.left

            root.leftMargin = 0 // resetting left margin in case of orientation switch
        } else {
            root.isLandscape = false
            outerGrid.rows = 3
            outerGrid.columns = 1
            // Three outer section width (base state, middle, plus button)
            baseStateWrapper.width = width
            root.scrollViewWidth = width
            addWrapper.width = width

            width -= doubleSizeExtension

            if (width > Constants.maxThumbSize) {
                // In this case we want to have a multi column grid in the center
                root.thumbSize = Constants.maxThumbSize

                let tmpScrollViewHeight = height - root.thumbSize * 1.5 - 2 * root.outerGridSpacing

                // Inner grid calculation
                numRows = Math.min(numStates, root.numFit(tmpScrollViewHeight, root.thumbSize,
                                                          root.innerGridSpacing))
                numColumns = root.numFit(width, Constants.maxThumbSize, root.innerGridSpacing)

                let tmpColumns = Math.ceil(numStates / numRows)

                if (tmpColumns <= numColumns)
                    numColumns = tmpColumns
                else
                    numRows = Math.ceil(numStates / numColumns)
            } else {
                // This case is for single column layout and small thumb view
                root.thumbSize = Math.max(width, Constants.minThumbSize)

                // Inner grid calculation
                numRows = numStates
                numColumns = 1
            }

            Constants.thumbnailSize = root.thumbSize

            let tmpHeight = root.thumbSize * numRows + root.innerGridSpacing * (numRows - 1) + doubleSizeExtension
            let remainingSpace = height - root.thumbSize - 2 * root.outerGridSpacing
            let space = remainingSpace - tmpHeight

            if (space >= root.thumbSize) {
                root.scrollViewHeight = tmpHeight
                addWrapper.height = space
            } else {
                addWrapper.height = Math.max(space, 0.5 * root.thumbSize)
                root.scrollViewHeight = remainingSpace - addWrapper.height
            }

            root.leftMargin = (root.scrollViewWidth - (root.thumbSize * numColumns)
                               - root.innerGridSpacing * (numColumns - 1)) * 0.5 - sizeExtension

            addCanvas.width = root.thumbSize
            addCanvas.height = Math.min(addWrapper.height, root.thumbSize)

            baseStateWrapper.height = root.thumbSize

            baseStateThumbnail.anchors.verticalCenter = undefined
            baseStateThumbnail.anchors.horizontalCenter = baseStateWrapper.horizontalCenter

            addCanvas.anchors.verticalCenter = undefined
            addCanvas.anchors.horizontalCenter = addWrapper.horizontalCenter
            addCanvas.anchors.top = addWrapper.top
            addCanvas.anchors.left = undefined

            root.topMargin = 0 // resetting top margin in case of orientation switch
        }

        // Always assign the bigger one first otherwise there will be console output complaining
        if (numRows > innerGrid.rows) {
            innerGrid.rows = numRows
            innerGrid.columns = numColumns
        } else {
            innerGrid.columns = numColumns
            innerGrid.rows = numRows
        }
    }

    // These function assume that the order of the states is as follows:
    // State A, State B (extends State A), ... so the extended state always comes first
    function isInRange(i) {
        return i >= 0 && i < StatesEditorBackend.statesEditorModel.count()
    }

    function nextStateHasExtend(i) {
        let next = i + 1
        return root.isInRange(next) ? StatesEditorBackend.statesEditorModel.get(next).hasExtend : false
    }

    function previousStateHasExtend(i) {
        let prev = i - 1
        return root.isInRange(prev) ? StatesEditorBackend.statesEditorModel.get(prev).hasExtend : false
    }

    property bool showExtendGroups: StatesEditorBackend.statesEditorModel.hasExtend

    onShowExtendGroupsChanged: root.responsiveResize(root.width, root.height)

    property int extend: 16

    property int thumbSize: 250

    property int padding: 10

    property int scrollViewWidth: 640
    property int scrollViewHeight: 480
    property int outerGridSpacing: 10
    property int innerGridSpacing: root.showExtendGroups ? 40 : root.outerGridSpacing

    // These margins are used to push the inner grid down or to the left depending on the views
    // orientation to align to the outer grid
    property int topMargin: 0
    property int leftMargin: 0

    property bool tinyMode: Constants.thumbnailSize <= Constants.thumbnailBreak

    property int currentStateInternalId: 0
    // Using an int instead of a bool, because when opening a menu on one state and without closing
    // opening a menu on another state will first trigger the open of the new popup and afterwards
    // the close of the old popup. Using an int keeps track of number of opened popups.
    property int menuOpen: 0

    Connections {
        target: StatesEditorBackend.statesEditorModel
        function onModelReset() {
            root.menuOpen = 0
            editDialog.close()
        }
    }

    // This timer is used to delay the current state animation as it didn't work due to the
    // repeaters item not being positioned in time resulting in 0 x and y position if the grids
    // row and column were not changed during the layout algorithm .
    Timer {
        id: layoutTimer
        interval: 50
        running: false
        repeat: false
        onTriggered: {
            // Move the current state into view if outside
            if (root.currentStateInternalId === 0)
                // Not for base state
                return

            var x = 0
            var y = 0
            for (var i = 0; i < statesRepeater.count; ++i) {
                let item = statesRepeater.itemAt(i)

                if (item.internalNodeId === root.currentStateInternalId) {
                    x = item.x
                    y = item.y
                    break
                }
            }

            // Check if it is in view
            if (x <= frame.contentX
                    || x >= (frame.contentX + root.scrollViewWidth - root.thumbSize))
                frame.contentX = x - root.scrollViewWidth * 0.5 + root.thumbSize * 0.5

            if (y <= frame.contentY
                    || y >= (frame.contentY + root.scrollViewHeight - root.thumbSize))
                frame.contentY = y - root.scrollViewHeight * 0.5 + root.thumbSize * 0.5
        }
    }

    onCurrentStateInternalIdChanged: layoutTimer.start()

    StudioControls.Dialog {
        id: editDialog
        title: qsTr("Rename state group")
        standardButtons: Dialog.Apply | Dialog.Cancel
        x: editButton.x - Math.max(0, editButton.x + editDialog.width - root.width)
        y: toolBar.height
        width: Math.min(300, root.width)
        closePolicy: Popup.NoAutoClose

        onApplied: editDialog.accept()

        StudioControls.TextField {
            id: editTextField
            actionIndicatorVisible: false
            translationIndicatorVisible: false
            anchors.fill: parent

            onTextChanged: {
                let btn = editDialog.standardButton(Dialog.Apply)
                if (!btn)
                    return

                if (editDialog.previousString !== editTextField.text) {
                    btn.enabled = true
                } else {
                    btn.enabled = false
                }
            }

            onAccepted: editDialog.accept()
            onRejected: editDialog.reject()
        }

        onAccepted: {
            let renamed = StatesEditorBackend.statesEditorModel.renameActiveStateGroup(editTextField.text)
            if (renamed)
                editDialog.close()
        }

        property string previousString

        onAboutToShow: {
            editTextField.text = StatesEditorBackend.statesEditorModel.activeStateGroup
            editDialog.previousString = StatesEditorBackend.statesEditorModel.activeStateGroup

            let btn = editDialog.standardButton(Dialog.Apply)
            btn.enabled = false
        }
    }

    Rectangle {
        id: toolBar

        property bool doubleRow: root.width < 450

        onDoubleRowChanged: {
            if (toolBar.doubleRow) {
                toolBarGrid.rows = 2
                toolBarGrid.columns = 1
            } else {
                toolBarGrid.columns = 2
                toolBarGrid.rows = 1
            }
        }

        color: StudioTheme.Values.themeToolbarBackground
        width: root.width
        height: (toolBar.doubleRow ? 2 : 1) * StudioTheme.Values.toolbarHeight

        Grid {
            id: toolBarGrid
            columns: 2
            rows: 1
            columnSpacing: StudioTheme.Values.toolbarSpacing

            Row {
                id: stateGroupSelectionRow
                height: StudioTheme.Values.toolbarHeight
                spacing: StudioTheme.Values.toolbarSpacing
                leftPadding: root.padding

                Text {
                    id: stateGroupLabel
                    color: StudioTheme.Values.themeTextColor
                    text: qsTr("State Group")
                    font.pixelSize: StudioTheme.Values.baseFontSize
                    horizontalAlignment: Text.AlignRight
                    verticalAlignment: Text.AlignVCenter
                    height: StudioTheme.Values.height
                    anchors.verticalCenter: parent.verticalCenter
                    visible: root.width > 240
                }

                StudioControls.ComboBox {
                    style: StudioTheme.Values.viewBarControlStyle
                    id: stateGroupComboBox
                    actionIndicatorVisible: false
                    model: StatesEditorBackend.statesEditorModel.stateGroups
                    currentIndex: StatesEditorBackend.statesEditorModel.activeStateGroupIndex
                    anchors.verticalCenter: parent.verticalCenter
                    width: stateGroupLabel.visible ? StudioTheme.Values.defaultControlWidth
                                                   : root.width - 2 * root.padding
                    enabled: !(StatesEditorBackend.statesEditorModel.isMCUs
                                && stateGroupComboBox.count <= 1)

                    HelperWidgets.Tooltip { id: comboBoxTooltip }

                    Timer {
                        interval: 1000
                        running: stateGroupComboBox.hovered
                        onTriggered: comboBoxTooltip.showText(stateGroupComboBox,
                                                              hoverHandler.point.position,
                                                              StatesEditorBackend.statesEditorModel.isMCUs
                                                                ? qsTr("State Groups are not supported with Qt for MCUs")
                                                                : qsTr("Switch State Group"))
                    }

                    onHoverChanged: {
                        if (!stateGroupComboBox.hovered)
                            comboBoxTooltip.hideText()
                    }

                    HoverHandler { id: hoverHandler }

                    popup.onOpened: editDialog.close()

                    // currentIndex needs special treatment, because if model is changed, it will be
                    // reset regardless of binding.
                    Connections {
                        target: StatesEditorBackend.statesEditorModel
                        function onActiveStateGroupIndexChanged() {
                            stateGroupComboBox.currentIndex = StatesEditorBackend.statesEditorModel.activeStateGroupIndex
                        }
                    }

                    onModelChanged: {
                        stateGroupComboBox.currentIndex = StatesEditorBackend.statesEditorModel.activeStateGroupIndex
                    }

                    onCompressedActivated: function (index, reason) {
                        StatesEditorBackend.statesEditorModel.activeStateGroupIndex = index
                        root.responsiveResize(root.width, root.height)
                    }
                }
            }

            Row {
                Row {
                    id: stateGroupEditRow
                    height: StudioTheme.Values.toolbarHeight
                    spacing: StudioTheme.Values.toolbarSpacing
                    leftPadding: toolBar.doubleRow ? root.padding : 0

                    HelperWidgets.AbstractButton {
                        style: StudioTheme.Values.viewBarButtonStyle
                        buttonIcon: StudioTheme.Constants.add_medium
                        anchors.verticalCenter: parent.verticalCenter
                        tooltip: StatesEditorBackend.statesEditorModel.isMCUs
                                    ? qsTr("State Groups are not supported with Qt for MCUs")
                                    : qsTr("Create State Group")
                        onClicked: StatesEditorBackend.statesEditorModel.addStateGroup("stateGroup")
                        enabled: !StatesEditorBackend.statesEditorModel.isMCUs
                    }

                    HelperWidgets.AbstractButton {
                        style: StudioTheme.Values.viewBarButtonStyle
                        buttonIcon: StudioTheme.Constants.remove_medium
                        anchors.verticalCenter: parent.verticalCenter
                        enabled: StatesEditorBackend.statesEditorModel.activeStateGroupIndex !== 0
                        tooltip: qsTr("Remove State Group")
                        onClicked: StatesEditorBackend.statesEditorModel.removeStateGroup()
                    }

                    HelperWidgets.AbstractButton {
                        id: editButton
                        style: StudioTheme.Values.viewBarButtonStyle
                        buttonIcon: StudioTheme.Constants.edit_medium
                        anchors.verticalCenter: parent.verticalCenter
                        enabled: StatesEditorBackend.statesEditorModel.activeStateGroupIndex !== 0
                        checked: editDialog.visible
                        tooltip: qsTr("Rename State Group")
                        onClicked: {
                            if (editDialog.opened)
                                editDialog.close()
                            else
                                editDialog.open()
                        }
                    }
                }

                Item {
                    width: Math.max(0, toolBar.width - (toolBar.doubleRow ? 0 : (stateGroupSelectionRow.width
                                    + toolBarGrid.columnSpacing))
                                    - stateGroupEditRow.width - thumbnailToggleRow.width)
                    height: 1
                }

                Row {
                    id: thumbnailToggleRow
                    height: StudioTheme.Values.toolbarHeight
                    spacing: StudioTheme.Values.toolbarSpacing
                    rightPadding: root.padding

                    HelperWidgets.AbstractButton {
                        style: StudioTheme.Values.viewBarButtonStyle
                        buttonIcon: StudioTheme.Constants.grid_medium
                        anchors.verticalCenter: parent.verticalCenter
                        enabled: !root.tinyMode
                        tooltip: qsTr("Show thumbnails")
                        onClicked: {
                            for (var i = 0; i < statesRepeater.count; ++i)
                                statesRepeater.itemAt(i).setPropertyChangesVisible(false)
                        }
                    }

                    HelperWidgets.AbstractButton {
                        style: StudioTheme.Values.viewBarButtonStyle
                        buttonIcon: StudioTheme.Constants.list_medium
                        anchors.verticalCenter: parent.verticalCenter
                        enabled: !root.tinyMode
                        tooltip: qsTr("Show property changes")
                        onClicked: {
                            for (var i = 0; i < statesRepeater.count; ++i)
                                statesRepeater.itemAt(i).setPropertyChangesVisible(true)
                        }
                    }
                }
            }
        }
    }

    Grid {
        id: outerGrid
        x: root.padding
        y: toolBar.height + root.padding
        columns: 3
        rows: 1
        spacing: root.outerGridSpacing

        Item {
            id: baseStateWrapper

            StateThumbnail {
                // Base State
                id: baseStateThumbnail
                width: Constants.thumbnailSize
                height: Constants.thumbnailSize
                baseState: true
                defaultChecked: !StatesEditorBackend.statesEditorModel.baseState.modelHasDefaultState // TODO Make this one a model property
                isChecked: root.currentStateInternalId === 0
                thumbnailImageSource: StatesEditorBackend.statesEditorModel.baseState.stateImageSource ?? "" // TODO Get rid of the QVariantMap
                isTiny: root.tinyMode

                onFocusSignal: root.currentStateInternalId = 0
                onDefaultClicked: StatesEditorBackend.statesEditorModel.resetDefaultState()
            }
        }

        Item {
            id: scrollViewWrapper
            width: root.isLandscape ? root.scrollViewWidth : root.width - (2 * root.padding)
            height: root.isLandscape ? root.height - toolBar.height - (2 * root.padding) : root.scrollViewHeight
            clip: true

            Basic.ScrollView {
                id: scrollView

                property bool adsFocus: false
                // objectName is used by the dock widget to find this particular ScrollView
                // and set the ads focus on it.
                objectName: "__mainSrollView"

                anchors.fill: parent
                anchors.topMargin: root.topMargin
                anchors.leftMargin: root.leftMargin

                Basic.ScrollBar.horizontal: StudioControls.TransientScrollBar {
                    id: horizontalBar
                    style: StudioTheme.Values.viewStyle
                    parent: scrollView
                    x: scrollView.leftPadding
                    y: scrollView.height - height
                    width: scrollView.availableWidth
                    orientation: Qt.Horizontal
                    visible: root.isLandscape

                    show: (scrollView.hovered || scrollView.focus || scrollView.adsFocus)
                          && horizontalBar.isNeeded
                    otherInUse: verticalBar.inUse
                }

                Basic.ScrollBar.vertical: StudioControls.TransientScrollBar {
                    id: verticalBar
                    style: StudioTheme.Values.viewStyle
                    parent: scrollView
                    x: scrollView.mirrored ? 0 : scrollView.width - width
                    y: scrollView.topPadding
                    height: scrollView.availableHeight
                    orientation: Qt.Vertical
                    visible: !root.isLandscape

                    show: (scrollView.hovered || scrollView.focus || scrollView.adsFocus)
                          && verticalBar.isNeeded
                    otherInUse: horizontalBar.inUse
                }

                Flickable {
                    id: frame
                    boundsMovement: Flickable.StopAtBounds
                    boundsBehavior: Flickable.StopAtBounds
                    interactive: true
                    contentWidth: {
                        let ext = root.showExtendGroups ? (2 * root.extend) : 0
                        return innerGrid.width + ext
                    }
                    contentHeight: {
                        let ext = root.showExtendGroups ? (2 * root.extend) : 0
                        return innerGrid.height + ext
                    }
                    flickableDirection: {
                        if (frame.contentHeight <= scrollView.height)
                            return Flickable.HorizontalFlick

                        if (frame.contentWidth <= scrollView.width)
                            return Flickable.VerticalFlick

                        return Flickable.HorizontalAndVerticalFlick
                    }

                    Behavior on contentY {
                        NumberAnimation {
                            duration: 1000
                            easing.type: Easing.InOutCubic
                        }
                    }

                    Behavior on contentX {
                        NumberAnimation {
                            duration: 1000
                            easing.type: Easing.InOutCubic
                        }
                    }

                    Grid {
                        id: innerGrid

                        x: root.showExtendGroups ? root.extend : 0
                        y: root.showExtendGroups ? root.extend : 0

                        rows: 1
                        spacing: root.innerGridSpacing

                        move: Transition {
                            NumberAnimation {
                                properties: "x,y"
                                easing.type: Easing.OutQuad
                                duration: 100
                            }
                        }

                        Repeater {
                            id: statesRepeater

                            property int grabIndex: -1

                            model: StatesEditorBackend.statesEditorModel

                            onItemAdded: root.responsiveResize(root.width, root.height)
                            onItemRemoved: root.responsiveResize(root.width, root.height)

                            delegate: DropArea {
                                id: delegateRoot

                                required property int index

                                required property string stateName
                                required property var stateImageSource
                                required property int internalNodeId
                                required property var hasWhenCondition
                                required property var whenConditionString
                                required property bool isDefault
                                required property var modelHasDefaultState
                                required property bool hasExtend
                                required property var extendString

                                function setPropertyChangesVisible(value) {
                                    stateThumbnail.setPropertyChangesVisible(value)
                                }

                                width: Constants.thumbnailSize
                                height: Constants.thumbnailSize

                                visible: delegateRoot.internalNodeId // Skip base state

                                property int visualIndex: index

                                onEntered: function (drag) {
                                    let dragSource = (drag.source as StateThumbnail)

                                    if (!dragSource)
                                        return

                                    if (dragSource.extendString !== stateThumbnail.extendString
                                            || stateThumbnail.extendedState) {
                                        return
                                    }

                                    StatesEditorBackend.statesEditorModel.move(dragSource.visualIndex,
                                                           stateThumbnail.visualIndex)
                                }

                                onDropped: function (drop) {
                                    let dropSource = (drop.source as StateThumbnail)

                                    if (!dropSource)
                                        return

                                    if (dropSource.extendString !== stateThumbnail.extendString
                                            || stateThumbnail.extendedState) {
                                        return
                                    }

                                    if (statesRepeater.grabIndex === dropSource.visualIndex)
                                        return

                                    StatesEditorBackend.statesEditorModel.drop(statesRepeater.grabIndex,
                                                           dropSource.visualIndex)
                                    statesRepeater.grabIndex = -1
                                }

                                // Extend Groups Visualization
                                Rectangle {
                                    id: extendBackground
                                    x: -root.extend
                                    y: -root.extend
                                    width: Constants.thumbnailSize + 2 * root.extend
                                    height: Constants.thumbnailSize + 2 * root.extend
                                    color: StudioTheme.Values.themeStateHighlight

                                    radius: {
                                        if (root.nextStateHasExtend(delegateRoot.index))
                                            return delegateRoot.hasExtend ? 0 : root.extend

                                        return root.extend
                                    }

                                    visible: (delegateRoot.hasExtend
                                              || stateThumbnail.extendedState)
                                }
                                // Fill the gap between extend group states and also cover up radius
                                // of start and end states of an extend group in case of line break
                                Rectangle {
                                    id: extendGap
                                    property bool portraitOneColumn: !root.isLandscape
                                                                     && innerGrid.columns === 1

                                    property bool leftOrTop: {
                                        if (delegateRoot.hasExtend)
                                            return true

                                        if (root.previousStateHasExtend(delegateRoot.index))
                                            return true

                                        return false
                                    }
                                    property bool rightOrBottom: {
                                        if (stateThumbnail.extendedState)
                                            return true

                                        if (root.nextStateHasExtend(delegateRoot.index))
                                            return true

                                        return false
                                    }

                                    property bool firstInRow: ((delegateRoot.index - 1) % innerGrid.columns) === 0
                                    property bool lastInRow: ((delegateRoot.index - 1) % innerGrid.columns)
                                                             === (innerGrid.columns - 1)

                                    x: {
                                        if (!extendGap.portraitOneColumn) {
                                            if (extendGap.rightOrBottom)
                                                return extendGap.lastInRow ? Constants.thumbnailSize
                                                                             - (root.innerGridSpacing
                                                                                - root.extend) : Constants.thumbnailSize
                                            if (extendGap.leftOrTop)
                                                return extendGap.firstInRow ? -root.extend : -root.innerGridSpacing

                                            return 0
                                        }

                                        return -root.extend
                                    }
                                    y: {
                                        if (extendGap.portraitOneColumn) {
                                            if (extendGap.rightOrBottom)
                                                return Constants.thumbnailSize
                                            if (extendGap.leftOrTop)
                                                return -root.innerGridSpacing

                                            return 0
                                        }

                                        return -root.extend
                                    }
                                    width: extendGap.portraitOneColumn ? Constants.thumbnailSize + 2
                                                                         * root.extend : root.innerGridSpacing
                                    height: extendGap.portraitOneColumn ? root.innerGridSpacing : Constants.thumbnailSize
                                                                          + 2 * root.extend
                                    color: StudioTheme.Values.themeStateHighlight
                                    visible: extendBackground.visible
                                }

                                StateThumbnail {
                                    id: stateThumbnail
                                    width: Constants.thumbnailSize
                                    height: Constants.thumbnailSize
                                    visualIndex: delegateRoot.visualIndex
                                    internalNodeId: delegateRoot.internalNodeId
                                    isTiny: root.tinyMode

                                    hasExtend: delegateRoot.hasExtend
                                    extendString: delegateRoot.extendString
                                    extendedState: StatesEditorBackend.statesEditorModel.extendedStates.includes(
                                                       delegateRoot.stateName)

                                    hasWhenCondition: delegateRoot.hasWhenCondition

                                    blockDragHandler: horizontalBar.active || verticalBar.active
                                                        || root.menuOpen

                                    dragParent: scrollView

                                    onMenuOpenChanged: {
                                        if (stateThumbnail.menuOpen)
                                            root.menuOpen++
                                        else
                                            root.menuOpen--
                                    }

                                    // Fix ScrollView taking over the dragging event
                                    onGrabbing: {
                                        frame.interactive = false
                                        statesRepeater.grabIndex = stateThumbnail.visualIndex
                                    }
                                    onLetGo: frame.interactive = true

                                    stateName: delegateRoot.stateName
                                    thumbnailImageSource: delegateRoot.stateImageSource
                                    whenCondition: delegateRoot.whenConditionString

                                    baseState: !delegateRoot.internalNodeId
                                    defaultChecked: delegateRoot.isDefault
                                    isChecked: root.currentStateInternalId === delegateRoot.internalNodeId

                                    onFocusSignal: root.currentStateInternalId = delegateRoot.internalNodeId
                                    onDefaultClicked: StatesEditorBackend.statesEditorModel.setStateAsDefault(
                                                          delegateRoot.internalNodeId)

                                    onClone: root.cloneState(delegateRoot.internalNodeId)
                                    onExtend: root.extendState(delegateRoot.internalNodeId)
                                    onRemove: root.deleteState(delegateRoot.internalNodeId)

                                    onStateNameFinished: StatesEditorBackend.statesEditorModel.renameState(
                                                             delegateRoot.internalNodeId,
                                                             stateThumbnail.stateName)
                                }
                            }
                        }
                    }
                }
            }
        }

        Item {
            id: addWrapper
            visible: StatesEditorBackend.statesEditorModel.canAddNewStates

            Canvas {
                id: addCanvas
                width: root.thumbWidth
                height: root.thumbHeight

                property int plusExtend: 20
                property int halfWidth: addCanvas.width / 2
                property int halfHeight: addCanvas.height / 2

                onPaint: {
                    var ctx = getContext("2d")

                    ctx.strokeStyle = StudioTheme.Values.themeStateHighlight
                    ctx.lineWidth = 6

                    ctx.beginPath()
                    ctx.moveTo(addCanvas.halfWidth, addCanvas.halfHeight - addCanvas.plusExtend)
                    ctx.lineTo(addCanvas.halfWidth, addCanvas.halfHeight + addCanvas.plusExtend)

                    ctx.moveTo(addCanvas.halfWidth - addCanvas.plusExtend, addCanvas.halfHeight)
                    ctx.lineTo(addCanvas.halfWidth + addCanvas.plusExtend, addCanvas.halfHeight)
                    ctx.stroke()

                    ctx.save()
                    ctx.setLineDash([2, 2])
                    ctx.strokeRect(0, 0, addCanvas.width, addCanvas.height)
                    ctx.restore()
                }

                MouseArea {
                    id: addMouseArea
                    anchors.fill: parent
                    hoverEnabled: true
                    onClicked: root.createNewState()
                }

                Rectangle {
                    // temporary hover indicator for add button
                    anchors.fill: parent
                    opacity: 0.1
                    color: addMouseArea.containsMouse ? "#ffffff" : "#000000"
                }
            }
        }
    }
}
