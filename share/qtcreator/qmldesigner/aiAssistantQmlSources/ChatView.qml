// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Controls
import StudioControls as StudioControls
import StudioTheme as StudioTheme
import AiGenerationState
import AiAssistantBackend

Rectangle {
    id: root

    property bool adsFocus: false

    required property var chatModel

    color: StudioTheme.Values.themeControlBackground
    border.color: StudioTheme.Values.themeControlOutline
    border.width: StudioTheme.Values.border
    radius: StudioTheme.Values.smallRadius

    ListView {
        id: listView

        anchors.fill: parent
        anchors.margins: 5
        anchors.rightMargin: 0
        spacing: 5
        clip: true

        model: root.chatModel
        delegate: ChatMessage {}
        footer: GenerationStateRow {}

        // Auto-scroll to bottom
        onCountChanged: {
            if (listView.count > 0)
                Qt.callLater(() => listView.positionViewAtEnd())
        }

        HoverHandler { id: hoverHandler }

        ScrollBar.vertical: StudioControls.TransientScrollBar {
            id: verticalScrollBar
            style: StudioTheme.Values.viewStyle
            parent: listView

            y: 0
            height: listView.availableHeight
            orientation: Qt.Vertical

            show: (hoverHandler.hovered || listView.focus || verticalScrollBar.inUse || root.adsFocus)
                  && verticalScrollBar.isNeeded
        }

        // Empty state
        Text {
            anchors.centerIn: parent
            visible: listView.count === 0
            text: qsTr("No conversation yet.\nStart by asking a question...")
            color: StudioTheme.Values.themeTextColorDisabled
            horizontalAlignment: Text.AlignHCenter
            font.pixelSize: StudioTheme.Values.baseFontSize
        }
    }
}
