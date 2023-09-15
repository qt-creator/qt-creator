// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import StudioTheme as StudioTheme

ItemDelegate {
    id: control

    property StudioTheme.ControlStyle style: StudioTheme.Values.controlStyle

    hoverEnabled: true
    verticalPadding: 0
    rightPadding: 10 // ScrollBar thickness

    contentItem: RowLayout {
        Text {
            Layout.fillWidth: true
            leftPadding: 8
            rightPadding: 8
            text: control.text
            font: control.font
            opacity: enabled ? 1.0 : 0.3
            color: control.hovered ? control.style.text.selectedText : control.style.text.idle
            horizontalAlignment: Text.AlignLeft
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
        }

        Item {
            visible: control.childCount
            width: 30
            height: 30

            Text {
                id: chevronLeft
                font.family: StudioTheme.Constants.iconFont.family
                font.pixelSize: control.style.baseIconFontSize
                color: control.hovered ? control.style.text.selectedText : control.style.text.idle
                text: StudioTheme.Constants.forward_medium
                anchors.centerIn: parent
            }
        }
    }

    background: Rectangle {
        implicitWidth: 200
        implicitHeight: 30
        opacity: enabled ? 1 : 0.3
        color: control.hovered ? control.style.interaction : "transparent"
    }
}
