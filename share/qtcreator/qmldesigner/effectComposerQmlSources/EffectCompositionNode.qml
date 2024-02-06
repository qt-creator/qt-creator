// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Controls
import HelperWidgets as HelperWidgets
import StudioControls as StudioControls
import StudioTheme as StudioTheme
import EffectComposerBackend

HelperWidgets.Section {
    id: root

    property int modelIndex: 0

    caption: nodeName
    category: "EffectComposer"

    draggable: !isDependency
    fillBackground: true
    showCloseButton: !isDependency
    closeButtonToolTip: qsTr("Remove")
    visible: repeater.count > 0 || !isDependency

    onCloseButtonClicked: {
        EffectComposerBackend.effectComposerModel.removeNode(root.modelIndex)
    }

    showEyeButton: !isDependency
    eyeEnabled: nodeEnabled
    eyeButtonToolTip: qsTr("Enable/Disable Node")

    onEyeButtonClicked: {
        nodeEnabled = root.eyeEnabled
    }

    Column {
        spacing: 10

        Repeater {
            id: repeater
            model: nodeUniformsModel

            EffectCompositionNodeUniform {
                width: root.width
            }
        }
    }
}

