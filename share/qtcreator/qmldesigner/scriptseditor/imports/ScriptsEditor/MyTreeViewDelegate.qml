// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Controls
import QtQuick.Templates as T
import StudioTheme as StudioTheme

T.TreeViewDelegate {
    id: control

    property StudioTheme.ControlStyle style: StudioTheme.Values.controlStyle

    hoverEnabled: true
    implicitWidth: 200
    implicitHeight: 30
    indentation: 12
    leftPadding: control.leftMargin + control.__contentIndent

    readonly property int customDepth: control.depth - 1

    readonly property real __contentIndent: !control.isTreeNode ? 0
                                                                : (control.customDepth * control.indentation)
                                                                  + (control.indicator ? control.indicator.width + control.spacing : 0)

    indicator: Item {
        x: control.leftMargin + (control.customDepth * control.indentation)
        width: 30
        height: 30

        Text {
            id: icon
            font.family: StudioTheme.Constants.iconFont.family
            font.pixelSize: control.style.smallIconFontSize
            color: control.hovered ? control.style.text.selectedText : control.style.text.idle
            text: StudioTheme.Constants.sectionToggle
            rotation: control.expanded ? 0 : -90
            anchors.centerIn: parent
        }
    }

    background: Rectangle {
        implicitWidth: 200
        implicitHeight: 30
        color: control.hovered ? control.style.interaction : "transparent"
    }

    contentItem: Text {
        text: control.text
        font: control.font
        opacity: enabled ? 1.0 : 0.3
        color: control.hovered ? control.style.text.selectedText : control.style.text.idle
        horizontalAlignment: Text.AlignLeft
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }
}
