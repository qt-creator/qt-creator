// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick 2.1
import QtQuick.Controls 1.1
import QtQuick.Controls.Private 1.0

Style {
    readonly property SpinBox control: __control

    property var __syspal: SystemPalette {
        colorGroup: control.enabled ?
                        SystemPalette.Active : SystemPalette.Disabled
    }

    padding {
       top: control.__panel ? control.__panel.topPadding + (styleitem.style === "mac" ? 2 : 0) : 0
       left: control.__panel ? control.__panel.leftPadding : 0
       right: control.__panel ? control.__panel.rightPadding : 0
       bottom: control.__panel ? control.__panel.bottomPadding : 0
   }
    StyleItem {id: styleitem ; visible: false}

    property int renderType: Text.NativeRendering

    property Component panel: Item {
        id: style

        property rect upRect
        property rect downRect

        property int horizontalAlignment: Qt.platform.os === "osx" ? Qt.AlignRight : Qt.AlignLeft
        property int verticalAlignment: Qt.AlignVCenter

        property color foregroundColor: __syspal.text
        property color backgroundColor: __syspal.base
        property color selectionColor: __syspal.highlight
        property color selectedTextColor: __syspal.highlightedText

        property int topPadding: edit.anchors.topMargin
        property int leftPadding: 3 + edit.anchors.leftMargin
        property int rightPadding: 3 + edit.anchors.rightMargin
        property int bottomPadding: edit.anchors.bottomMargin

        width: 100
        height: styleitem.implicitHeight

        implicitWidth: 2 + styleitem.implicitWidth
        implicitHeight: styleitem.implicitHeight
        baselineOffset: styleitem.baselineOffset

        Item {
            id: edit
            anchors.fill: parent
            Rectangle {
                color: "white"
                anchors.fill: parent
                anchors.margins: -1
            }
            FocusFrame {
                anchors.fill: parent
                focusMargin:-6
                visible: spinbox.activeFocus && styleitem.styleHint("focuswidget")
            }
        }

        function updateRect() {
            style.upRect = styleitem.subControlRect("up");
            style.downRect = styleitem.subControlRect("down");
            var inputRect = styleitem.subControlRect("edit");
            edit.anchors.topMargin = inputRect.y
            edit.anchors.leftMargin = inputRect.x
            edit.anchors.rightMargin = style.width - inputRect.width - edit.anchors.leftMargin
            edit.anchors.bottomMargin = style.height - inputRect.height - edit.anchors.topMargin
        }

        Component.onCompleted: updateRect()
        onWidthChanged: updateRect()
        onHeightChanged: updateRect()

        StyleItem {
            id: styleitem
            elementType: "spinbox"
            anchors.fill: parent
            sunken: (styleData.downEnabled && styleData.downPressed) || (styleData.upEnabled && styleData.upPressed)
            hover: control.hovered
            hints: control.styleHints
            hasFocus: control.activeFocus
            enabled: control.enabled
            value: (styleData.upPressed ? 1 : 0)           |
                   (styleData.downPressed ? 1<<1 : 0) |
                   (styleData.upEnabled ? (1<<2) : 0)      |
                   (styleData.downEnabled ? (1<<3) : 0)
            contentWidth: styleData.contentWidth
            contentHeight: styleData.contentHeight
            textureHeight: implicitHeight
            border {top: 6 ; bottom: 6}
        }
    }
}
