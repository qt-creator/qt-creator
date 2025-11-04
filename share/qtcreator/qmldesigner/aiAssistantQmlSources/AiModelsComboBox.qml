// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

pragma ComponentBehavior: Bound

import QtQuick
import StudioControls as StudioControls
import QtQuick.Templates as T
import QtQuick.Controls as QQC
import AiAssistantBackend

StudioControls.ComboBox {
    id: control

    readonly property real delegateWidth: control.popup.width - control.popup.leftPadding - control.popup.rightPadding

    model: AiAssistantBackend.rootView.modelsModel
    textRole: "modelId"
    actionIndicatorVisible: false

    Binding on currentIndex {
        value: control.model.selectedIndex
        delayed: true
    }

    onCurrentIndexChanged: {
        control.model.selectedIndex = control.currentIndex
    }

    popup: T.Popup {
        id: comboBoxPopup
        x: control.actionIndicator.width
        y: control.height
        width: control.width - control.actionIndicator.width
        height: Math.min(comboBoxPopup.contentItem.implicitHeight + comboBoxPopup.topPadding
                         + comboBoxPopup.bottomPadding,
                         control.Window.height - comboBoxPopup.topMargin - comboBoxPopup.bottomMargin,
                         control.maximumPopupHeight)
        padding: control.style.borderWidth
        margins: 0 // If not defined margin will be -1
        closePolicy: T.Popup.CloseOnPressOutside | T.Popup.CloseOnPressOutsideParent
                     | T.Popup.CloseOnEscape | T.Popup.CloseOnReleaseOutside
                     | T.Popup.CloseOnReleaseOutsideParent

        contentItem: ListView {
            id: listView

            implicitWidth: control.width
            implicitHeight: 200
            clip: true

            model: control.model
            section.property: "provider"
            section.criteria: ViewSection.FullString
            boundsBehavior: Flickable.StopAtBounds

            QQC.ScrollBar.vertical: StudioControls.TransientScrollBar {
                id: verticalScrollBar
                parent: listView
                x: listView.width - verticalScrollBar.width
                y: 0
                height: listView.availableHeight
                orientation: Qt.Vertical

                show: (listViewHoverHandler.hovered || verticalScrollBar.inUse)
                      && verticalScrollBar.isNeeded
            }

            HoverHandler { id: listViewHoverHandler }

            delegate: T.ItemDelegate {
                id: listDelegate

                required property int index
                required property string modelId
                readonly property bool isSelected: listDelegate.index === control.currentIndex

                text: listDelegate.modelId
                implicitWidth: listView.width

                hoverEnabled: true
                implicitHeight: 30

                background: Rectangle {
                    color: {
                        if (listDelegate.hovered && listDelegate.isSelected)
                            return control.style.interactionHover

                        if (listDelegate.isSelected)
                            return control.style.interaction

                        if (listDelegate.hovered)
                            return control.style.background.hover

                        return "transparent"
                    }
                }

                contentItem: Text {
                    text: listDelegate.text
                    font: control.font
                    color: listDelegate.isSelected ? control.style.text.selectedText
                                                   : control.style.text.idle
                    verticalAlignment: Text.AlignVCenter
                    elide: Text.ElideRight
                    leftPadding: 15
                }

                onClicked: {
                    control.currentIndex = listDelegate.index
                    comboBoxPopup.close()
                }
            }

            section.delegate: Text {
                id: sectionDelegate

                required property string section

                text: sectionDelegate.section
                color: control.style.text.idle
                verticalAlignment: Text.AlignBottom
                bottomPadding: 5
                font.family: control.font.family
                font.bold: true
                height: 30
            }
        }

        background: Rectangle {
            color: control.style.popup.background
            border.width: 0
        }
    }
}
