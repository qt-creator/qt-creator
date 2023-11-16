// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Controls
import QtQuickDesignerTheme
import HelperWidgets as HelperWidgets
import StudioControls as StudioControls
import StudioTheme 1.0 as StudioTheme
import EffectMakerBackend

HelperWidgets.Section {
    id: root

    caption: nodeName
    category: "EffectMaker"

    draggable: true
    fillBackground: true
    showCloseButton: true
    closeButtonToolTip: qsTr("Remove")

    onCloseButtonClicked: {
        EffectMakerBackend.effectMakerModel.removeNode(root.index)
    }

    showEyeButton: true
    eyeEnabled: nodeEnabled
    eyeButtonToolTip: qsTr("Enable/Disable Node")

    onEyeButtonClicked: {
        nodeEnabled = root.eyeEnabled
    }

    Column {
        spacing: 10

        Repeater {
            model: nodeUniformsModel

            EffectCompositionNodeUniform {
                width: root.width
            }
        }
    }
}

