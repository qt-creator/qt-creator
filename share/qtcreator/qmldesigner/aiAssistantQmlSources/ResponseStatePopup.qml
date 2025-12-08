// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts
import HelperWidgets as HelperWidgets
import StudioTheme as StudioTheme
import AiAssistantBackend

Rectangle {
    id: root

    border.color: StudioTheme.Values.themeControlOutlineHover
    border.width: 1
    radius: 3

    implicitHeight: successRow.implicitHeight + 10 // 10: vertical padding (5 top + 5 bottom)

    color: "transparent"
    visible: false

    function hidePopup() {
        root.visible = false
        timer.stop()
    }

    Connections {
        target: AiAssistantBackend.rootView

        function onNotifyAIResponseSuccess() {
            messageText.text = qsTr("Done! How does this look?")
            icon.icon = StudioTheme.Constants.apply_small
            icon.iconColor = StudioTheme.Values.notification_successDefault
            successRow.visible = true
            wrongQmlRow.visible = false
            errorRow.visible = false

            root.visible = true
            timer.restart()
        }

        function onNotifyAIResponseInvalidQml() {
            messageText.text = qsTr("The generated Qml has errors, would you like to show the result?")
            icon.icon = StudioTheme.Constants.warning_medium
            icon.iconColor = StudioTheme.Values.notification_alertDefault
            successRow.visible = false
            wrongQmlRow.visible = true
            errorRow.visible = false

            root.visible = true
        }

        function onNotifyAIResponseError(errMessage) {
            messageText.text = errMessage
            icon.icon = StudioTheme.Constants.error_medium
            icon.iconColor = StudioTheme.Values.notification_dangerDefault
            successRow.visible = false
            wrongQmlRow.visible = false
            errorRow.visible = true

            root.visible = true
            timer.restart()
        }

        function onIsGeneratingChanged() {
            if (AiAssistantBackend.rootView.isGenerating)
                root.hidePopup()
        }

        function onRemoveFeedbackPopup() {
            hidePopup()
        }
    }

    Timer {
        id: timer

        interval: 30000
        onTriggered: root.visible = false
    }

    RowLayout {
        x: 5 // left margin
        anchors.verticalCenter: parent.verticalCenter
        width: parent.width - 10

        HelperWidgets.IconLabel {
            id: icon
        }

        Text {
            id: messageText

            color: StudioTheme.Values.themeTextColor
            wrapMode: Text.WordWrap

            Layout.fillWidth: true
        }

        Row {
            id: successRow

            visible: false
            spacing: 2

            AiIconButton {
                id: thumbsUpButton
                objectName: "ThumbsUpButton"

                buttonIcon: StudioTheme.Constants.thumbs_up_medium

                tooltip: qsTr("Like")

                onClicked: {
                    AiAssistantBackend.rootView.sendThumbFeedback(true)
                    root.hidePopup()
                }
            }

            AiIconButton {
                id: thumbsDownButton
                objectName: "ThumbsDownButton"

                buttonIcon: StudioTheme.Constants.thumbs_down_medium

                tooltip: qsTr("Dislike")

                onClicked: {
                    AiAssistantBackend.rootView.sendThumbFeedback(false)
                    root.hidePopup()
                }
            }

            AiIconButton {
                id: undoChangeButton
                objectName: "UndoChangeButton"

                buttonIcon: StudioTheme.Constants.undo

                tooltip: qsTr("Undo change")

                onClicked: {
                    AiAssistantBackend.rootView.undoLastChange()
                    root.hidePopup()
                }
            }

            AiIconButton {
                id: closeFeedbackButton
                objectName: "CloseFeedbackButton"

                buttonIcon: StudioTheme.Constants.adsClose

                tooltip: qsTr("Close")

                onClicked: root.hidePopup()
            }
        }

        Row {
            id: wrongQmlRow

            visible: false
            spacing: 2

            PromptButton {
                id: yesButton
                objectName: "YesButton"

                label: qsTr("Yes")
                tooltip: qsTr("Load the erroneous QML code.")

                onClicked: {
                    AiAssistantBackend.rootView.applyLastGeneratedQml()

                    root.hidePopup()
                }
            }

            PromptButton {
                id: noButton
                objectName: "NoButton"

                label: qsTr("No")
                tooltip: qsTr("Don't load the erroneous QML code.")

                onClicked: root.hidePopup()
            }
        }

        Row {
            id: errorRow

            visible: false
            spacing: 2

            PromptButton {
                id: retryButton
                objectName: "RetryButton"

                label: qsTr("Retry")
                tooltip: qsTr("Retry last prompt.")

                onClicked: {
                    AiAssistantBackend.rootView.retryLastPrompt()

                    root.hidePopup()
                }
            }
        }
    }
}
