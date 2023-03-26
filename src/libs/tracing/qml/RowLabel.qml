// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Controls
import QtCreator.Tracing

Button {
    id: button
    property var label

    readonly property int dragHeight: 5

    signal selectBySelectionId()
    signal setRowHeight(int newHeight)

    property string labelText: label.description ? label.description
                                                 : qsTranslate("QtC::Tracing", "[unknown]")

    onPressed: selectBySelectionId();
    ToolTip.text: labelText + (label.displayName ? (" (" + label.displayName + ")") : "")
    ToolTip.visible: hovered
    ToolTip.delay: 1000

    background: Rectangle {
        border.width: 1
        border.color: Theme.color(Theme.Timeline_DividerColor)
        color: Theme.color(Theme.PanelStatusBarBackgroundColor)
    }

    contentItem: TimelineText {
        text: button.labelText
        verticalAlignment: Text.AlignVCenter
        horizontalAlignment: Text.AlignLeft
        elide: Text.ElideRight
        color: Theme.color(Theme.PanelTextColorLight)
    }

    MouseArea {
        hoverEnabled: true
        property bool resizing: false
        onPressed: resizing = true
        onReleased: resizing = false

        height: button.dragHeight
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        cursorShape: Qt.SizeVerCursor

        onMouseYChanged: {
            if (resizing)
                button.setRowHeight(y + mouseY)
        }
    }
}

