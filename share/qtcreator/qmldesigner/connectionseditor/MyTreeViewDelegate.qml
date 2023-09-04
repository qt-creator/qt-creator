// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Controls
import QtQuick.Templates as T
import StudioTheme as StudioTheme

T.TreeViewDelegate {
    id: control
    hoverEnabled: true

    implicitWidth: 200
    implicitHeight: 30

    //implicitWidth: leftMargin + __contentIndent + implicitContentWidth + rightPadding + rightMargin
    //implicitHeight: Math.max(indicator ? indicator.height : 0, implicitContentHeight) * 1.25

    indentation: 12
    //leftMargin: 4
    //rightMargin: 4
    //spacing: 4

    //topPadding: contentItem ? (height - contentItem.implicitHeight) / 2 : 0
    leftPadding: control.leftMargin + control.__contentIndent

    //required property int row
    //required property var model
    readonly property real __contentIndent: !control.isTreeNode ? 0
                                                                : (control.depth * control.indentation)
                                                                  + (control.indicator ? control.indicator.width + control.spacing : 0)

    indicator: Item {
        readonly property real __indicatorIndent: control.leftMargin + (control.depth * control.indentation)

        x: __indicatorIndent
        width: 30
        height: 30

        Text {
            id: caret
            font.family: StudioTheme.Constants.iconFont.family
            font.pixelSize: StudioTheme.Values.smallIconFontSize
            color: control.hovered ? "#111111" : "white" // TODO colors
            text: StudioTheme.Constants.sectionToggle
            rotation: control.expanded ? 0 : -90
            anchors.centerIn: parent
        }
    }

    background: Rectangle {
        implicitWidth: 200
        implicitHeight: 30
        color: control.hovered ? "#4DBFFF" : "transparent"
    }

    contentItem: Text {
        text: control.text
        font: control.font
        opacity: enabled ? 1.0 : 0.3
        color: control.hovered ? "#111111" : "white"
        horizontalAlignment: Text.AlignLeft
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }
}
