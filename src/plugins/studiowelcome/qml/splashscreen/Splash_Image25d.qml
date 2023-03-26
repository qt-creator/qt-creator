// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
import QtQuick 2.3

Rectangle {
    id: splashBackground
    width: 460
    height: 480
    color: "transparent"
    scale: 1.2

    layer.enabled: true
    layer.textureSize: Qt.size(width * 2, height * 2)
    layer.smooth: true

    Item {
        id: composition
        anchors.centerIn: parent
        width: 460
        height: 480
        visible: true
        anchors.verticalCenterOffset: -1
        anchors.horizontalCenterOffset: 14
        clip: true

        layer.enabled: true
        layer.textureSize: Qt.size(width * 2, height * 2)
        layer.smooth: true

        Splash_Image2d_png {
            x: -22
            y: -33
            width: 461
            height: 427
            layer.enabled: true
            layer.effect: ColorOverlayEffect {
                id: colorOverlay
                visible: true
                color: "#41cd52"
            }
            scale: 1
        }
    }
}
