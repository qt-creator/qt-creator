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
    height: 22

    color: mouseArea.containsMouse ? StudioTheme.Values.themeControlBackgroundInteraction
                                   : "transparent"

    MouseArea {
        id: mouseArea

        anchors.fill: parent
        hoverEnabled: true
        acceptedButtons: Qt.LeftButton

        onClicked: {
            EffectMakerBackend.rootView.addEffectNode(modelData.nodeName)
        }
    }

    Row {
        spacing: 5

        IconImage {
            id: nodeIcon

            width: 22
            height: 22

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
