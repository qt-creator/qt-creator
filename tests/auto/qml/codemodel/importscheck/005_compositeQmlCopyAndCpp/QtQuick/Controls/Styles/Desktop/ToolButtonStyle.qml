// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick 2.1
import QtQuick.Controls 1.1
import QtQuick.Controls.Private 1.0

Style {
    property Component panel: StyleItem {
        id: styleitem

        anchors.fill: parent
        elementType: "toolbutton"
        on: control.checkable && control.checked
        sunken: control.pressed
        raised: !(control.checkable && control.checked) && control.hovered
        hover: control.hovered
        hasFocus: control.activeFocus
        hints: control.styleHints
        text: control.text

        properties: {
            "icon": control.__iconAction.__icon,
            "position": control.__position,
            "menu" : control.menu !== null
        }
    }
}
