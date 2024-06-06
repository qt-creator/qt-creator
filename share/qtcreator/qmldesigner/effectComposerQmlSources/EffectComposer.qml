// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Templates as T
import HelperWidgets as HelperWidgets
import StudioControls as StudioControls
import StudioTheme as StudioTheme
import EffectComposerBackend

ColumnLayout {
    id: root

    spacing: 1

    readonly property var backendModel: EffectComposerBackend.effectComposerModel

    property var draggedSec: null
    property var secsY: []
    property int moveFromIdx: 0
    property int moveToIdx: 0
    property bool previewAnimationRunning: false
    property var expandStates: null

    // Invoked after save changes is done
    property var onSaveChangesCallback: () => {}

    // Invoked from C++ side when open composition is requested and there are unsaved changes
    function promptToSaveBeforeOpen() {
        root.onSaveChangesCallback = () => { EffectComposerBackend.rootView.doOpenComposition() }

        saveChangesDialog.open()
    }

    // Invoked from C++ side before resetting the model to store current expanded state of nodes
    function storeExpandStates() {
        expandStates = new Map()

        for (let i = 0; i < repeater.count; ++i) {
            var curItem = repeater.itemAt(i)
            expandStates.set(curItem.caption, curItem.expanded)
        }
    }

    // Invoked after model has been reset to restore expanded state for nodes
    function restoreExpandStates() {
        if (expandStates) {
            for (let i = 0; i < repeater.count; ++i) {
                var curItem = repeater.itemAt(i)
                if (expandStates.has(curItem.caption))
                    curItem.expanded = expandStates.get(curItem.caption)
            }
            expandStates = null
        }
    }

    Connections {
        target: root.backendModel
        function onIsEmptyChanged() {
            if (root.backendModel.isEmpty)
                saveAsDialog.close()
        }
    }

    SaveAsDialog {
        id: saveAsDialog
        anchors.centerIn: parent
    }

    SaveChangesDialog {
        id: saveChangesDialog
        anchors.centerIn: parent

        onSave: {
            if (root.backendModel.currentComposition === "") {
                // if current composition is unsaved, show save as dialog and clear afterwards
                saveAsDialog.clearOnClose = true
                saveAsDialog.open()
            } else {
                root.onSaveChangesCallback()
            }
        }

        onDiscard: {
            root.onSaveChangesCallback()
        }
    }

    ConfirmClearAllDialog {
        id: confirmClearAllDialog
        anchors.centerIn: parent
    }

    EffectComposerTopBar {
        Layout.fillWidth: true

        onAddClicked: {
            root.onSaveChangesCallback = () => { root.backendModel.clear(true) }

            if (root.backendModel.hasUnsavedChanges)
                saveChangesDialog.open()
            else
                root.backendModel.clear(true)
        }

        onSaveClicked: {
            let name = root.backendModel.currentComposition

            if (name === "")
                saveAsDialog.open()
            else
                root.backendModel.saveComposition(name)
        }

        onSaveAsClicked: saveAsDialog.open()

        onAssignToSelectedClicked: {
            root.backendModel.assignToSelected()
        }
    }

    SplitView {
        id: splitView

        Layout.fillWidth: true
        Layout.fillHeight: true

        orientation: root.width > root.height ? Qt.Horizontal : Qt.Vertical

        handle: Rectangle {
            implicitWidth: splitView.orientation === Qt.Horizontal ? StudioTheme.Values.splitterThickness : splitView.width
            implicitHeight: splitView.orientation === Qt.Horizontal ? splitView.height : StudioTheme.Values.splitterThickness
            color: T.SplitHandle.pressed ? StudioTheme.Values.themeSliderHandleInteraction
                : (T.SplitHandle.hovered ? StudioTheme.Values.themeSliderHandleHover
                                         : "transparent")
        }

        EffectComposerPreview {
            mainRoot: root

            SplitView.minimumWidth: 250
            SplitView.minimumHeight: 200
            SplitView.preferredWidth: 300
            SplitView.preferredHeight: 300
            Layout.fillWidth: true
            Layout.fillHeight: true

            FrameAnimation {
                id: previewFrameTimer
                running: true
                paused: !previewAnimationRunning
            }
        }

        Column {
            spacing: 1

            SplitView.minimumWidth: 250
            SplitView.minimumHeight: 100

            Component.onCompleted: HelperWidgets.Controller.mainScrollView = scrollView

            Rectangle {
                width: parent.width
                height: StudioTheme.Values.toolbarHeight
                color: StudioTheme.Values.themeToolbarBackground

                EffectNodesComboBox {
                    id: nodesComboBox

                    mainRoot: root

                    anchors.verticalCenter: parent.verticalCenter
                    x: 5
                    width: parent.width - 50
                }

                HelperWidgets.AbstractButton {
                    anchors.right: parent.right
                    anchors.rightMargin: 5
                    anchors.verticalCenter: parent.verticalCenter

                    style: StudioTheme.Values.viewBarButtonStyle
                    buttonIcon: StudioTheme.Constants.clearList_medium
                    tooltip: qsTr("Remove all effect nodes.")
                    enabled: root.backendModel ? !root.backendModel.isEmpty : false

                    onClicked: {
                        if (root.backendModel.hasUnsavedChanges)
                            confirmClearAllDialog.open()
                        else
                            root.backendModel.clear()
                    }
                }

                HelperWidgets.AbstractButton {
                    anchors.right: parent.right
                    anchors.rightMargin: 5
                    anchors.verticalCenter: parent.verticalCenter

                    style: StudioTheme.Values.viewBarButtonStyle
                    buttonIcon: StudioTheme.Constants.code
                    tooltip: qsTr("Open Shader in Code Editor.")
                    visible: false // TODO: to be implemented

                    onClicked: {} // TODO
                }
            }

            Item {
                width: parent.width
                height: parent.height - y

                HelperWidgets.ScrollView {
                    id: scrollView

                    readonly property int dragScrollMargin: 50

                    anchors.fill: parent
                    clip: true
                    interactive: !HelperWidgets.Controller.contextMenuOpened

                    onContentHeightChanged: {
                        // Expand states are stored before full model reset.
                        // Content height change indicates the model has been updated after full
                        // reset, so we restore expand states if any are stored.
                        root.restoreExpandStates()

                        // If content height change was because a recent node addition, we want to
                        // scroll to the end of the content so the newly added item is visible.
                        if (nodesComboBox.nodeJustAdded && scrollView.contentItem.height > scrollView.height) {
                            let lastItemH = repeater.itemAt(repeater.count - 1).height
                            scrollView.contentY = scrollView.contentItem.height - lastItemH
                            nodesComboBox.nodeJustAdded = false
                        }
                    }

                    Column {
                        id: nodesCol
                        width: scrollView.width
                        spacing: 1

                        Repeater {
                            id: repeater

                            width: parent.width
                            model: root.backendModel

                            onCountChanged: {
                                HelperWidgets.Controller.setCount("EffectComposer", repeater.count)
                            }

                            delegate: EffectCompositionNode {
                                width: parent.width
                                modelIndex: index

                                property bool wasExpanded: false

                                Behavior on y {
                                    id: dragAnimation
                                    PropertyAnimation {
                                        duration: 300
                                        easing.type: Easing.InOutQuad
                                    }
                                }

                                onStartDrag: (section) => {
                                    root.draggedSec = section
                                    root.moveFromIdx = index
                                    // We only need to animate non-dragged sections
                                    dragAnimation.enabled = false
                                    wasExpanded = expanded
                                    expanded = false
                                    highlightBorder = true
                                    root.secsY = []
                                }

                                onStopDrag: {
                                    if (root.secsY.length !== 0) {
                                        if (root.moveFromIdx === root.moveToIdx)
                                            root.draggedSec.y = root.secsY[root.moveFromIdx]
                                        else
                                            root.backendModel.moveNode(root.moveFromIdx, root.moveToIdx)
                                    }

                                    highlightBorder = false
                                    root.draggedSec = null
                                    expanded = wasExpanded
                                    dragAnimation.enabled = true
                                }
                            }
                        } // Repeater
                    } // Column
                } // ScrollView

                Text {
                    text: root.backendModel ? root.backendModel.isEnabled
                                              ? qsTr("Add an effect node to start")
                                              : qsTr("Effect Composer is disabled on MCU projects")
                                            : ""
                    color: StudioTheme.Values.themeTextColor
                    font.pixelSize: StudioTheme.Values.baseFontSize

                    anchors.centerIn: parent

                    visible: root.backendModel ? root.backendModel.isEmpty : false
                }
            } // Item
        } // Column
    } // SplitView

    function handleDragMove() {
        dragTimer.stop()
        if (root.secsY.length === 0) {
            for (let i = 0; i < repeater.count; ++i)
                root.secsY[i] = repeater.itemAt(i).y
        }

        let oldContentY = scrollView.contentY
        if (root.draggedSec.y < scrollView.dragScrollMargin + scrollView.contentY
                && scrollView.contentY > 0) {
            scrollView.contentY -= scrollView.dragScrollMargin / 2
        } else if (root.draggedSec.y > scrollView.contentY + scrollView.height - scrollView.dragScrollMargin
                    && scrollView.contentY < scrollView.contentHeight - scrollView.height) {
            scrollView.contentY += scrollView.dragScrollMargin / 2
            if (scrollView.contentY > scrollView.contentHeight - scrollView.height)
                scrollView.contentY = scrollView.contentHeight - scrollView.height
        }
        if (scrollView.contentY < 0)
            scrollView.contentY = 0

        if (oldContentY !== scrollView.contentY) {
            // Changing dragged section position in drag handler doesn't seem to stick
            // when triggered by mouse move, so do it again async
            dragTimer.targetY = root.draggedSec.y - oldContentY + scrollView.contentY
            dragTimer.restart()
            dragConnection.enabled = false
            root.draggedSec.y = dragTimer.targetY
            dragConnection.enabled = true
        }

        root.moveToIdx = root.moveFromIdx
        for (let i = 0; i < repeater.count; ++i) {
            let currItem = repeater.itemAt(i)
            if (i > root.moveFromIdx) {
                if (root.draggedSec.y > currItem.y) {
                    currItem.y = root.secsY[i] - root.draggedSec.height - nodesCol.spacing
                    root.moveToIdx = i
                } else {
                    currItem.y = root.secsY[i]
                }
            } else if (i < root.moveFromIdx) {
                if (!repeater.model.isDependencyNode(i) && root.draggedSec.y < currItem.y) {
                    currItem.y = root.secsY[i] + root.draggedSec.height + nodesCol.spacing
                    root.moveToIdx = Math.min(root.moveToIdx, i)
                } else {
                    currItem.y = root.secsY[i]
                }
            }
        }
    }

    Connections {
        id: dragConnection
        target: root.draggedSec
        function onYChanged() { root.handleDragMove() }
    }

    Timer {
        id: dragTimer
        running: false
        interval: 16
        repeat: false

        property real targetY: -1

        onTriggered: {
            // Ensure we get position change triggers even if user holds mouse still to
            // make scrolling smooth
            root.draggedSec.y = targetY
            root.handleDragMove()
        }
    } // Timer
}
