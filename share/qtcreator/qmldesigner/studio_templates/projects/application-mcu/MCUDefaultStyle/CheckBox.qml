/******************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Quick Ultralite module.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
******************************************************************************/

import QtQuick 2.15
import QtQuick.Templates 2.15 as T

T.CheckBox {
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
                ? "images/checkbox-disabled.png"
                : (control.checkState == Qt.PartiallyChecked
                   ? (control.down ? "images/checkbox-partially-checked-pressed.png" : "images/checkbox-partially-checked.png")
                   : (control.checkState == Qt.Checked
                      ? (control.down ? "images/checkbox-checked-pressed.png" : "images/checkbox-checked.png")
                      : "images/checkbox.png"))
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
