// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick 2.1
import QtQuick.Controls 1.1
import QtQuick.Controls.Private 1.0


Style {
    readonly property GroupBox control: __control

    property var __style: StyleItem { id: style }
    property int titleHeight: 18

    Component.onCompleted: {
        var stylename = __style.style
        if (stylename.indexOf("windows") > -1)
            titleHeight = 9
    }

    padding {
        top: Math.round(Settings.dpiScaleFactor * (control.title.length > 0 || control.checkable ? titleHeight : 0) + (style.style == "mac" ? 9 : 6))
        left: Math.round(Settings.dpiScaleFactor * 8)
        right: Math.round(Settings.dpiScaleFactor * 8)
        bottom: Math.round(Settings.dpiScaleFactor * 7 + (style.style.indexOf("windows") > -1 ? 2 : 0))
    }

    property Component panel: StyleItem {
        anchors.fill: parent
        id: styleitem
        elementType: "groupbox"
        text: control.title
        on: control.checked
        hasFocus: control.__checkbox.activeFocus
        activeControl: control.checkable ? "checkbox" : ""
        properties: { "checkable" : control.checkable , "sunken" : !control.flat}
        textureHeight: 128
        border {top: 32 ; bottom: 8}
    }
}
