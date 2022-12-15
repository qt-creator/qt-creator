// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0 WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Templates as T
import StudioTheme 1.0 as StudioTheme

T.DialogButtonBox {
    id: control

    property StudioTheme.ControlStyle style: StudioTheme.Values.controlStyle

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            (control.count === 1 ? implicitContentWidth * 2 : implicitContentWidth) + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding)
    contentWidth: contentItem.contentWidth

    spacing: control.style.dialogButtonSpacing
    padding: control.style.dialogPadding
    alignment: Qt.AlignRight | Qt.AlignBottom


    delegate: DialogButton {
        style: control.style
        width: control.count === 1 ? control.availableWidth / 2 : undefined
        implicitHeight: control.style.controlSize.height
        highlighted: DialogButtonBox.buttonRole === DialogButtonBox.AcceptRole
                     || DialogButtonBox.buttonRole === DialogButtonBox.ApplyRole
    }

    contentItem: ListView {
        implicitWidth: contentWidth
        model: control.contentModel
        spacing: control.spacing
        orientation: ListView.Horizontal
        boundsBehavior: Flickable.StopAtBounds
        snapMode: ListView.SnapToItem
    }

    background: Rectangle {
        implicitHeight: 30
        x: control.style.borderWidth
        y: control.style.borderWidth
        width: parent.width - (2 * control.style.borderWidth)
        height: parent.height - (2 * control.style.borderWidthr)
        color: control.style.dialog.background
    }
}
