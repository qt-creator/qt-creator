// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Controls
import QtQuickDesignerTheme
import HelperWidgets
import StudioControls as StudioControls
import StudioTheme 1.0 as StudioTheme
import EffectMakerBackend

Rectangle {
    id: root

    width: 140
    height: 32

    color: mouseArea.containsMouse && modelData.canBeAdded
           ? StudioTheme.Values.themeControlBackgroundInteraction : "transparent"

    signal addEffectNode(var nodeQenPath)

    ToolTipArea {
        id: mouseArea

        anchors.fill: parent
        acceptedButtons: Qt.LeftButton

        tooltip: modelData.canBeAdded ? "" : qsTr("Existing effect has conflicting properties, this effect cannot be added.")

        onClicked: {
            if (modelData.canBeAdded)
                root.addEffectNode(modelData.nodeQenPath)
        }
    }

    Row {
        spacing: 5

        IconImage {
            id: nodeIcon

            width: 32
            height: 32

            color: modelData.canBeAdded ? StudioTheme.Values.themeTextColor
                                        : StudioTheme.Values.themeTextColorDisabled
            source: modelData.nodeIcon
        }

        Text {
            text: modelData.nodeName
            color: modelData.canBeAdded ? StudioTheme.Values.themeTextColor
                                        : StudioTheme.Values.themeTextColorDisabled
            font.pointSize: StudioTheme.Values.smallFontSize
            anchors.verticalCenter: nodeIcon.verticalCenter
        }
    }
}
