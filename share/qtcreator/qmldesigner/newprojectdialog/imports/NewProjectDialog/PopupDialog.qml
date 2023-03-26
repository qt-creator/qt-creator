// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import StudioTheme as StudioTheme

import BackendApi

Dialog {
    id: root
    padding: DialogValues.popupDialogPadding

    background: Rectangle {
        color: DialogValues.darkPaneColor
        border.color: StudioTheme.Values.themeInteraction
        border.width: StudioTheme.Values.border
    }

    header: Label {
        text: root.title
        visible: root.title
        elide: Label.ElideRight
        font.bold: true
        font.pixelSize: DialogValues.defaultPixelSize
        padding: DialogValues.popupDialogPadding
        color: DialogValues.textColor
        horizontalAlignment: Text.AlignHCenter

        background: Rectangle {
            x: 1
            y: 1
            width: parent.width - 2
            height: parent.height - 1
            color: DialogValues.darkPaneColor
        }
    }

    footer: PopupDialogButtonBox {
        visible: count > 0
    }
}
