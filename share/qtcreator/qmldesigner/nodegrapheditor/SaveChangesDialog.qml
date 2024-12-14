// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Controls
import HelperWidgets as HelperWidgets
import StudioControls as StudioControls
import StudioTheme as StudioTheme
import EffectComposerBackend

StudioControls.Dialog {
    id: root

    signal discard
    signal save

    closePolicy: Popup.CloseOnEscape
    implicitHeight: 130
    implicitWidth: 300
    modal: true
    title: qsTr("Save Changes")

    contentItem: Item {
        Text {
            color: StudioTheme.Values.themeTextColor
            text: qsTr("Current node graph has unsaved changes.")
        }

        HelperWidgets.Button {
            anchors.bottom: parent.bottom
            text: qsTr("Cancel")
            width: 60

            onClicked: root.reject()
        }

        Row {
            anchors.bottom: parent.bottom
            anchors.right: parent.right
            spacing: 2

            HelperWidgets.Button {
                text: qsTr("Save")
                width: 50

                onClicked: {
                    root.save();
                    root.accept();
                }
            }

            HelperWidgets.Button {
                text: qsTr("Discard Changes")
                width: 110

                onClicked: {
                    root.discard();
                    root.accept();
                }
            }
        }
    }
}
