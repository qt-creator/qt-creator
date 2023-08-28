// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Templates as T

T.ProgressBar {
    id: root

    implicitWidth: Math.max((background ? background.implicitWidth : 0) + leftInset + rightInset,
                            (contentItem ? contentItem.implicitWidth : 0) + leftPadding + rightPadding)
    implicitHeight: Math.max((background ? background.implicitHeight : 0) + topInset + bottomInset,
                             (contentItem ? contentItem.implicitHeight : 0) + topPadding + bottomPadding)

    leftInset: DefaultStyle.inset
    topInset: DefaultStyle.inset
    rightInset: DefaultStyle.inset
    bottomInset: DefaultStyle.inset
    leftPadding: DefaultStyle.padding
    topPadding: DefaultStyle.padding
    rightPadding: DefaultStyle.padding
    bottomPadding: DefaultStyle.padding
    background: Item {
        implicitWidth: 200
        implicitHeight: 16
        BorderImage {
            width: parent.width; height: 16
            anchors.verticalCenter: parent.verticalCenter
            border.left: 7
            border.right: 7
            border.top: 7
            border.bottom: 7
            source: "images/progressbar-background.png"
        }
    }
    contentItem: Item {
        implicitWidth: 200
        implicitHeight: 16
        BorderImage {
            width: parent.width * root.position; height: 16
            anchors.verticalCenter: parent.verticalCenter
            border.left: 6
            border.right: 6
            border.top: 7
            border.bottom: 7
            source: "images/progressbar-progress.png"
        }
    }
}
