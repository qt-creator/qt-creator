// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0 WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Templates as T
import StudioTheme 1.0 as StudioTheme

T.Dialog {
    id: control

    property StudioTheme.ControlStyle style: StudioTheme.Values.controlStyle

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            contentWidth + leftPadding + rightPadding,
                            implicitHeaderWidth,
                            implicitFooterWidth)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             contentHeight + topPadding + bottomPadding
                             + (implicitHeaderHeight > 0 ? implicitHeaderHeight + spacing : 0)
                             + (implicitFooterHeight > 0 ? implicitFooterHeight + spacing : 0))

    padding: control.style.dialogPadding

    background: Rectangle {
        color: control.style.dialog.background
        border.color: control.style.dialog.border
        border.width: control.style.borderWidth
    }

    header: T.Label {
        text: control.title
        visible: control.title
        elide: T.Label.ElideRight
        font.bold: true
        padding: control.padding
        color: control.style.text.idle

        background: Rectangle {
            x: control.style.borderWidth
            y: control.style.borderWidth
            width: parent.width - (2 * control.style.borderWidth)
            height: parent.height - (2 * control.style.borderWidth)
            color: control.style.dialog.background
        }
    }

    footer: DialogButtonBox {
        style: control.style
        visible: count > 0
    }

    T.Overlay.modal: Rectangle {
        color: Qt.alpha(control.style.dialog.overlay, 0.5)
    }
}
