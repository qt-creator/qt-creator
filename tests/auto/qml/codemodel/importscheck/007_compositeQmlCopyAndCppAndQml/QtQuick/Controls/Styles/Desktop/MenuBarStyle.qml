// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick 2.1
import QtQuick.Controls 1.1
import QtQuick.Controls.Private 1.0


Style {
    property Component frame: StyleItem {
        elementType: "menubar"
        contentWidth: control.__contentItem.width
        contentHeight: parent ? parent.contentHeight : 0
        width: implicitWidth + 2 * (pixelMetric("menubarhmargin") + pixelMetric("menubarpanelwidth"))
        height: implicitHeight + 2 * (pixelMetric("menubarvmargin") + pixelMetric("menubarpanelwidth"))
                + pixelMetric("spacebelowmenubar")

        Accessible.role: Accessible.MenuBar
    }

    property Component menuItem: StyleItem {
        elementType: "menubaritem"
        x: pixelMetric("menubarhmargin") + pixelMetric("menubarpanelwidth")
        y: pixelMetric("menubarvmargin") + pixelMetric("menubarpanelwidth")

        text: menuItem.title
        contentWidth: textWidth(text)
        contentHeight: textHeight(text)
        width: implicitWidth + pixelMetric("menubaritemspacing")

        enabled: menuItem.enabled
        selected: (parent && parent.selected) || sunken
        sunken: parent && parent.sunken

        hints: { "showUnderlined": showUnderlined }

        Accessible.role: Accessible.MenuItem
        Accessible.name: StyleHelpers.removeMnemonics(text)
    }
}
