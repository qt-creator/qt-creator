// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Templates as T

T.RadioButton {
    id: control

    implicitWidth: Math.max((background ? background.implicitWidth : 0) + leftInset + rightInset,
                            (contentItem ? contentItem.implicitWidth : 0) + leftPadding + rightPadding)
    // TODO: https://bugreports.qt.io/browse/UL-783
//    implicitHeight: Math.max((background ? background.implicitHeight : 0) + topInset + bottomInset,
//                             (contentItem ? contentItem.implicitHeight : 0) + topPadding + bottomPadding,
//                             (indicator ? indicator.implicitHeight : 0) + topPadding + bottomPadding)
    implicitHeight: Math.max((background ? background.implicitHeight : 0) + topInset + bottomInset,
                             Math.max((contentItem ? contentItem.implicitHeight : 0) + topPadding + bottomPadding,
                             (indicator ? indicator.implicitHeight : 0) + topPadding + bottomPadding))

    leftPadding: DefaultStyle.padding
    rightPadding: DefaultStyle.padding
    topPadding: DefaultStyle.padding
    bottomPadding: DefaultStyle.padding

    leftInset: DefaultStyle.inset
    topInset: DefaultStyle.inset
    rightInset: DefaultStyle.inset
    bottomInset: DefaultStyle.inset

    spacing: 10

    indicator: Image {
        x: control.leftPadding
        y: control.topPadding + (control.availableHeight - height) / 2
        source: !control.enabled
                ? "images/radiobutton-disabled.png"
                : (control.checked
                   ? (control.down ? "images/radiobutton-checked-pressed.png" : "images/radiobutton-checked.png")
                   : (control.down ? "images/radiobutton-pressed.png" : "images/radiobutton.png"))
    }

    // Need a background for insets to work.
    background: Item {
        implicitWidth: 100
        implicitHeight: 42
    }

    contentItem: Text {
        text: control.text
        font: control.font
        color: control.enabled ? DefaultStyle.textColor : DefaultStyle.disabledTextColor
        verticalAlignment: Text.AlignVCenter
        leftPadding: control.indicator ? (control.indicator.width + control.spacing) : 0
    }
}
