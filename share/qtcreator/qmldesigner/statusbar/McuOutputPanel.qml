// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import StudioControls as StudioControls
import StudioTheme as StudioTheme

import OutputPane

ScrollView {
    id: root
    clip: true

    property var model: McuOutputModel {
        id: mcuOutputModel
    }

    function clearOutput() {
        model.clear()
    }

    function scrollDown() {
        const f = root.contentItem;
        f.contentY = Math.max(0, f.contentHeight - f.height);
    }

    onVisibleChanged: {
        scrollDown()
    }

    ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

    ScrollBar.vertical: StudioControls.TransientScrollBar {
        id: verticalScrollBar
        style: StudioTheme.Values.viewStyle
        parent: root
        x: root.width - verticalScrollBar.width
        y: 0
        height: root.availableHeight
        orientation: Qt.Vertical
        show: (root.hovered || root.focus) && verticalScrollBar.isNeeded
    }

    Text {
        width: root.width
        text: mcuOutputModel.text
        textFormat: Text.RichText
        wrapMode: Text.Wrap
        color: StudioTheme.Values.themeTextColor
        font.pixelSize: StudioTheme.Values.baseFontSize

        onTextChanged: {
            scrollDown()
        }
    }
}
