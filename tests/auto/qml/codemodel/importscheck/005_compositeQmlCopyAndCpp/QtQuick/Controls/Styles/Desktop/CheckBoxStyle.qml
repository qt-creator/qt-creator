// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick 2.1
import QtQuick.Controls 1.1
import QtQuick.Controls.Private 1.0

Style {
    property Component panel: Item {
        anchors.fill: parent

        implicitWidth: styleitem.implicitWidth
        implicitHeight: styleitem.implicitHeight
        baselineOffset: styleitem.baselineOffset
        StyleItem {
            id: styleitem
            elementType: "checkbox"
            sunken: control.pressed
            on: control.checked || control.pressed
            hover: control.hovered
            enabled: control.enabled
            hasFocus: control.activeFocus && styleitem.style == "mac"
            hints: control.styleHints
            properties: {"partiallyChecked": (control.checkedState === Qt.PartiallyChecked) }
            contentHeight: textitem.implicitHeight
            contentWidth: textitem.implicitWidth + indicatorWidth
            property int indicatorWidth: pixelMetric("indicatorwidth") + (macStyle ? 2 : 4)
            property bool macStyle: (style === "mac")

            Text {
                id: textitem
                text: control.text
                anchors.left: parent.left
                anchors.leftMargin: parent.indicatorWidth
                anchors.verticalCenter: parent.verticalCenter
                anchors.verticalCenterOffset: parent.macStyle ? 1 : 0
                anchors.right: parent.right
                renderType: Text.NativeRendering
                elide: Text.ElideRight
                enabled: control.enabled
                color: __syspal.windowText
                StyleItem {
                    elementType: "focusrect"
                    anchors.margins: -1
                    anchors.leftMargin: -2
                    anchors.top: parent.top
                    anchors.left: parent.left
                    anchors.bottom: parent.bottom
                    width: textitem.implicitWidth + 3
                    visible: control.activeFocus
                }
            }
        }
    }
}
