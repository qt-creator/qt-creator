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

    color: mouseArea.containsMouse ? StudioTheme.Values.themeControlBackgroundInteraction
                                   : "transparent"

    signal addEffectNode(var nodeQenPath)

    MouseArea {
        id: mouseArea

        anchors.fill: parent
        hoverEnabled: true
        acceptedButtons: Qt.LeftButton

        onClicked: {
            root.addEffectNode(modelData.nodeQenPath)
        }
    }

    Row {
        spacing: 5

        IconImage {
            id: nodeIcon

            width: 32
            height: 32

            color: StudioTheme.Values.themeTextColor
            source: modelData.nodeIcon
        }

        Text {
            text: modelData.nodeName
            color: StudioTheme.Values.themeTextColor
            font.pointSize: StudioTheme.Values.smallFontSize
            anchors.verticalCenter: nodeIcon.verticalCenter
        }
    }
}
