// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick 2.1
import QtQuick.Controls 1.1
import QtQuick.Controls.Private 1.0

ScrollViewStyle {
    property font font: __styleitem.font
    property color textColor: __styleitem.textColor
    property color selectionColor: __syspal.highlight
    property color selectedTextColor: __syspal.highlightedText
    property color backgroundColor: control.backgroundVisible ? __syspal.base : "transparent"

    property StyleItem __styleitem: StyleItem{
        property color textColor: styleHint("textColor")
        elementType: "edit"
        visible: false
        active: control.activeFocus
        onActiveChanged: textColor = styleHint("textColor")
    }

    property int renderType: Text.NativeRendering
}
