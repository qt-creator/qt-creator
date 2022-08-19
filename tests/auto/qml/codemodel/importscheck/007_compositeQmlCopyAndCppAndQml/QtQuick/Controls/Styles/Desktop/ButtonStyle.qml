// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick 2.1
import QtQuick.Controls 1.1
import QtQuick.Controls.Private 1.0

Style {
    property Component panel: StyleItem {
        id: styleitem
        elementType: "button"
        sunken: control.pressed || (control.checkable && control.checked)
        raised: !(control.pressed || (control.checkable && control.checked))
        hover: control.hovered
        text: control.iconSource === "" ? "" : control.text
        hasFocus: control.activeFocus
        hints: control.styleHints
        // If no icon, let the style do the drawing
        activeControl: control.isDefault ? "default" : "f"

        properties: {
            "icon": control.__iconAction.__icon,
            "menu": control.menu
        }
    }
}
