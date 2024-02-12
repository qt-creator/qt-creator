// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Controls
import HelperWidgets as HelperWidgets
import StudioControls as StudioControls
import StudioTheme as StudioTheme
import EffectComposerBackend

StudioControls.Dialog {
    id: root

    title: qsTr("Confirm clear list")

    closePolicy: Popup.CloseOnEscape
    modal: true
    implicitWidth: 270
    implicitHeight: 150

    contentItem: Item {
        Text {
            text: qsTr("You are about to clear the list of effect nodes.\n\nThis can not be undone.")
            color: StudioTheme.Values.themeTextColor
        }

        Row {
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            spacing: 2

            HelperWidgets.Button {
                text: qsTr("Clear")
                onClicked: {
                    EffectComposerBackend.effectComposerModel.clear()
                    root.accept()
                }
            }

            HelperWidgets.Button {
                anchors.bottom: parent.bottom
                text: qsTr("Cancel")
                onClicked: root.reject()
            }
        }
    }
}
