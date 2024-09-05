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

    signal openShadersCodeEditor(index: int)

    onEyeButtonClicked: {
        nodeEnabled = root.eyeEnabled
    }

    icons: HelperWidgets.IconButton {
        icon: StudioTheme.Constants.codeEditor_medium
        transparentBg: true
        buttonSize: 21
        iconSize: StudioTheme.Values.smallIconFontSize
        iconColor: StudioTheme.Values.themeTextColor
        iconScale: containsMouse ? 1.2 : 1
        implicitWidth: width
        onClicked: root.openShadersCodeEditor(index)
    }

    content: Label {
        text: root.caption
        color: root.labelColor
        elide: Text.ElideRight
        font.pixelSize: root.sectionFontSize
        font.capitalization: root.labelCapitalization
        anchors.verticalCenter: parent?.verticalCenter
        textFormat: Text.RichText
        leftPadding: StudioTheme.Values.toolbarSpacing
    }

    Column {
        spacing: 10

        Repeater {
            id: repeater
            model: nodeUniformsModel

            EffectCompositionNodeUniform {
                width: root.width

                onReset: nodeUniformsModel.resetData(index)
            }
        }
    }
}

