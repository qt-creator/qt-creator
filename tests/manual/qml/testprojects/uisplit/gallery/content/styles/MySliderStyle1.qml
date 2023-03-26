// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.2
import QtQuick.Controls 1.2
import QtQuick.Controls.Styles 1.1
import QtQuick.Particles 2.0
import QtQuick.Layouts 1.0

SliderStyle {
    groove: BorderImage {
        height: 6
        border.top: 1
        border.bottom: 1
        source: "../../images/progress-background.png"
        border.left: 6
        border.right: 6
        BorderImage {
            anchors.verticalCenter: parent.verticalCenter
            source: "../../images/progress-fill.png"
            border.left: 5 ; border.top: 1
            border.right: 5 ; border.bottom: 1
            width: styleData.handlePosition
            height: parent.height
        }
    }
    handle: Item {
        width: 13
        height: 13
        Image {
            anchors.centerIn: parent
            source: "../../images/slider-handle.png"
        }
    }
}
