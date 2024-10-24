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

    readonly property var backendModel: EffectComposerBackend.effectComposerModel
    readonly property bool codeEditorOpen: root.backendModel.codeEditorIndex === root.modelIndex

    property int modelIndex: 0

    caption: nodeName
    category: "EffectComposer"

    draggable: !isDependency
    fillBackground: true
    showCloseButton: !isDependency
    closeButtonToolTip: qsTr("Remove")
    visible: repeater.count > 0 || !isDependency

    onCloseButtonClicked: {
        root.backendModel.removeNode(root.modelIndex)
    }

    showEyeButton: !isDependency
    eyeEnabled: nodeEnabled
    eyeButtonToolTip: qsTr("Enable/Disable Node")

    onEyeButtonClicked: {
        nodeEnabled = root.eyeEnabled
    }

    icons: HelperWidgets.IconButton {
        id: codeButton

        icon: StudioTheme.Constants.codeEditor_medium
        transparentBg: true
        buttonSize: 21
        iconSize: StudioTheme.Values.smallIconFontSize
        iconColor: root.codeEditorOpen
                   ? StudioTheme.Values.themeInteraction
                   : StudioTheme.Values.themeTextColor
        iconScale: codeButton.containsMouse ? 1.2 : 1
        implicitWidth: width
        tooltip: qsTr("Open code editor")
        onClicked: root.backendModel.openCodeEditor(index)
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

