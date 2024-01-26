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

    title: qsTr("Save Changes")

    closePolicy: Popup.CloseOnEscape
    modal: true
    implicitWidth: 300
    implicitHeight: 130

    signal save()
    signal discard()

    contentItem: Item {
        Text {
            text: qsTr("Current composition has unsaved changes.")
            color: StudioTheme.Values.themeTextColor
        }

        HelperWidgets.Button {
            width: 60
            anchors.bottom: parent.bottom
            text: qsTr("Cancel")
            onClicked: root.reject()
        }

        Row {
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            spacing: 2

            HelperWidgets.Button {
                width: 50
                text: qsTr("Save")
                onClicked: {
                    let name = EffectComposerBackend.effectComposerModel.currentComposition
                    if (name !== "")
                        EffectComposerBackend.effectComposerModel.saveComposition(name)

                    root.save()
                    root.accept()
                }
            }

            HelperWidgets.Button {
                width: 110
                text: qsTr("Discard Changes")
                onClicked: {
                    root.discard()
                    root.accept()
                }
            }
        }
    }
}
