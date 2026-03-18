// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import StudioTheme as StudioTheme
import AiGenerationState
import AiAssistantBackend

Row {
    id: root

    visible: AiAssistantBackend.rootView.generationState != GenerationState.Idle
    spacing: 2
    leftPadding: 5
    bottomPadding: 5

    Text {
        text: {
            const state = AiAssistantBackend.rootView.generationState

            switch (state) {
            case GenerationState.CallingTool:
                return qsTr("Using tool")

            case GenerationState.ProcessingTool:
                return qsTr("Processing")

            case GenerationState.WaitingForConsent:
                return qsTr("Waiting for your response")

            default:
                return qsTr("Generating")
            }
        }

        color: StudioTheme.Values.themeTextColor
        font.pixelSize: StudioTheme.Values.baseFontSize
        anchors.verticalCenter: parent.verticalCenter
    }

    Repeater {
        model: 3

        Text {
            required property int index

            text: "."
            color: StudioTheme.Values.themeTextColor
            font.pixelSize: StudioTheme.Values.baseFontSize
            anchors.verticalCenter: parent.verticalCenter

            SequentialAnimation on opacity {
                running: AiAssistantBackend.rootView.generationState != GenerationState.Idle
                loops: Animation.Infinite

                PauseAnimation { duration: index * 300 }
                NumberAnimation { from: 0.2; to: 1.0; duration: 400; easing.type: Easing.InOutSine }
                NumberAnimation { from: 1.0; to: 0.2; duration: 400; easing.type: Easing.InOutSine }
                PauseAnimation { duration: (2 - index) * 300 }
            }
        }
    }
}
