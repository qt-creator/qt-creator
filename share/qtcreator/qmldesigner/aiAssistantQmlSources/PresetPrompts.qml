// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import HelperWidgets as HelperWidgets
import StudioControls as StudioControls
import StudioTheme as StudioTheme
import AiAssistantBackend

RowLayout {
    id: root

    readonly property var presetPrompts: ["Add a rectangle", "Add a button", "Add a Text", "Create a sample UI", "Remove all objects"]
    property int splitIndex: presetPrompts.length

    property var rootView: AiAssistantBackend.rootView
    readonly property bool isReady: !root.rootView.isGenerating && root.rootView.hasValidModel && root.rootView.termsAccepted

    signal triggered(prompt: string)

    Text { // use as TextMetrics, as the latter doesn't report as accurate values as an actual Text
        id: textMetrics
        visible: false
        font.pixelSize: StudioTheme.Values.baseFontSize
    }

    function updateSplit()
    {
        let rightEdge = root.width - (overflowButton.visible ? overflowButton.width : 0)
        let accWidth = 0 // width from left edge to item i right edge
        for (let i = 0; i < presetPrompts.length; ++i) {
            textMetrics.text = presetPrompts[i]
            accWidth += textMetrics.width + 12 + (i > 0 ? row.spacing : 0) // 12: PromptButton margins X

            if (accWidth > rightEdge) {
                root.splitIndex = i
                return
            }
        }
        root.splitIndex = presetPrompts.length
    }

    onWidthChanged: Qt.callLater(root.updateSplit) // Defer calculation to avoid recursive layout recalculation warning

    Row {
        id: row

        spacing: 3
        Layout.fillWidth: true

        Repeater {
            id: repeater

            model: root.presetPrompts.slice(0, root.splitIndex)

            delegate: PromptButton {
                required property string modelData

                label: modelData
                enabled: root.isReady

                onClicked: root.triggered(modelData)
            }
        }
    }

    HelperWidgets.AbstractButton {
        id: overflowButton
        objectName: "OverflowButton"

        style: StudioTheme.Values.viewBarButtonStyle
        buttonIcon: StudioTheme.Constants.more_medium
        iconSize: 12
        width: 24
        height: 24

        tooltip: qsTr("More preset prompts")

        visible: root.splitIndex < root.presetPrompts.length

        onClicked: {
            overflowMenu.popup(root, root.width - overflowMenu.width - 5, overflowButton.height + 5)
        }
    }

    StudioControls.Menu {
        id: overflowMenu
        closePolicy: Popup.CloseOnPressOutside | Popup.CloseOnEscape

        Instantiator {
            model: root.presetPrompts.slice(root.splitIndex)
            delegate: StudioControls.MenuItem {
                required property string modelData
                text: modelData
                enabled: root.isReady
                onTriggered: {
                    root.triggered(modelData)
                    overflowMenu.close()
                }
            }
            onObjectAdded: (index, object) => overflowMenu.insertItem(index, object)
            onObjectRemoved: (index, object) => overflowMenu.removeItem(object)
        }
    }
}
