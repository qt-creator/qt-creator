// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick 2.1
import QtQuick.Controls 1.1
import QtQuick.Controls.Private 1.0

Style {
    property Component panel: StyleItem {
        anchors.fill: parent
        elementType: "progressbar"
        // XXX: since desktop uses int instead of real, the progressbar
        // range [0..1] must be stretched to a good precision
        property int factor : 1000
        property int decimals: 3
        value:   indeterminate ? 0 : control.value.toFixed(decimals) * factor // does indeterminate value need to be 1 on windows?
        minimum: indeterminate ? 0 : control.minimumValue.toFixed(decimals) * factor
        maximum: indeterminate ? 0 : control.maximumValue.toFixed(decimals) * factor
        enabled: control.enabled
        horizontal: control.orientation === Qt.Horizontal
        hints: control.styleHints
        contentWidth: horizontal ? 200 : 23
        contentHeight: horizontal ? 23 : 200
    }
}
