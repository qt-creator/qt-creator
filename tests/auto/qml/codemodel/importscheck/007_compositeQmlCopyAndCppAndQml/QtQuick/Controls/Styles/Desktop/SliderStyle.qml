// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick 2.1
import QtQuick.Controls 1.1
import QtQuick.Controls.Private 1.0

Style {
    readonly property Item control: __control
    property Component panel: StyleItem {
        elementType: "slider"
        sunken: control.pressed
        implicitWidth: 200
        contentHeight: horizontal ? 22 : 200
        contentWidth: horizontal ? 200 : 22

        maximum: control.maximumValue*100
        minimum: control.minimumValue*100
        step: control.stepSize*100
        value: control.__handlePos*100
        horizontal: control.orientation === Qt.Horizontal
        enabled: control.enabled
        hasFocus: control.activeFocus
        hints: control.styleHints
        activeControl: control.tickmarksEnabled ? "ticks" : ""
        property int handleWidth: 15
        property int handleHeight: 15
    }
    padding { top: 0 ; left: 0 ; right: 0 ; bottom: 0 }
}
