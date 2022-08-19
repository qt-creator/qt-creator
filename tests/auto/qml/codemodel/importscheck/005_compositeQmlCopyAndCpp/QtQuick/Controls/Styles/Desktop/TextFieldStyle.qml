// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick 2.1
import QtQuick.Controls 1.1
import QtQuick.Controls.Private 1.0

Style {
    property int renderType: Text.NativeRendering

    property Component panel: StyleItem {
        id: textfieldstyle
        elementType: "edit"
        anchors.fill: parent

        sunken: true
        hasFocus: control.activeFocus
        hover: hovered
        hints: control.styleHints

        SystemPalette {
            id: syspal
            colorGroup: control.enabled ?
                            SystemPalette.Active :
                            SystemPalette.Disabled
        }

        property color textColor: syspal.text
        property color placeholderTextColor: "darkGray"
        property color selectionColor: syspal.highlight
        property color selectedTextColor: syspal.highlightedText


        property bool rounded: !!hints["rounded"]
        property int topMargin: style === "mac" ? 3 : 2
        property int leftMargin: rounded ? 12 : 4
        property int rightMargin: leftMargin
        property int bottomMargin: 2

        contentWidth: 100
        // Form QLineEdit::sizeHint
        contentHeight: Math.max(control.__contentHeight, 16)

        FocusFrame {
            anchors.fill: parent
            visible: textfield.activeFocus && textfieldstyle.styleHint("focuswidget") && !rounded
        }
        textureHeight: implicitHeight
        textureWidth: 32
        border {top: 8 ; bottom: 8 ; left: 8 ; right: 8}
    }
}
