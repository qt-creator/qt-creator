// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Templates as T

T.Switch {
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

    indicator: Item {
        x: control.leftPadding
        y: control.topPadding + (control.availableHeight - height) / 2
        width: 100
        height: 42

        Rectangle {
            id: colorRect

            anchors.fill: parent
            radius: 4
            color: checked ? "#5ae0aa" : "#c4c9cc"
        }

        Image {
            x: parent.width * 0.25 - width / 2
            source: "images/switch-i.png"
            anchors.verticalCenter: parent.verticalCenter
        }

        Image {
            x: parent.width * 0.75 - width / 2
            source: "images/switch-o.png"
            anchors.verticalCenter: parent.verticalCenter
        }

        Image {
            source: "images/switch-handle.png"
            // 8 is the space on each side of the handle in the image.
            x: Math.max(-8, Math.min((parent.width - width) + 8,
                control.visualPosition * parent.width - (width / 2)))
            anchors.verticalCenter: parent.verticalCenter
            anchors.verticalCenterOffset: 5

            Behavior on x { NumberAnimation { duration: 150 } }
        }
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
