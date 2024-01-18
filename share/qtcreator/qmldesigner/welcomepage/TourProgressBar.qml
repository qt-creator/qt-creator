// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Controls
import WelcomeScreen 1.0

Item {
    id: progressBar

    property int endSlide: 10
    property int currentSlide: 1

    Rectangle {
        id: progressBarGroove
        color: "#272727"
        radius: 5
        border.color: "#00000000"
        anchors.fill: parent
    }

    Rectangle {
        id: progressBarTrack
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 1
        anchors.topMargin: 1
        width: (progressBarGroove.width / 100) * rangeMapper.output
        color: "#57b9fc"
        radius: 4.5
    }

    RangeMapper {
        id: rangeMapper
        inputMaximum: progressBar.endSlide
        outputMaximum: 100
        outputMinimum: 0
        inputMinimum: 0
        input: progressBar.currentSlide
    }
}
