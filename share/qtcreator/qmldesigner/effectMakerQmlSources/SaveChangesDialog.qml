// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Controls
import HelperWidgets as HelperWidgets
import StudioControls as StudioControls
import StudioTheme as StudioTheme
import EffectMakerBackend

StudioControls.Dialog {
    id: root

    title: qsTr("Save Changes")

    closePolicy: Popup.CloseOnEscape
    modal: true
    implicitWidth: 250
    implicitHeight: 140

    contentItem: Column {
        spacing: 35

        Text {
            text: qsTr("Current composition has unsaved changes.")
            color: StudioTheme.Values.themeTextColor
        }

        Row {
            anchors.right: parent.right
            spacing: 2

            HelperWidgets.Button {
                id: btnSave

                width: 70
                text: qsTr("Save")
                onClicked: {
                    if (btnSave.enabled) {
                        let name = EffectMakerBackend.effectMakerModel.currentComposition
                        EffectMakerBackend.effectMakerModel.saveComposition(name)
                        root.accept()
                    }
                }
            }

            HelperWidgets.Button {
                id: btnDontSave

                width: 70
                text: qsTr("Don't Save")
                onClicked: root.accept()
            }

            HelperWidgets.Button {
                width: 70
                text: qsTr("Cancel")
                onClicked: root.reject()
            }
        }
    }
}
