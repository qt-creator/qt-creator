// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick 2.1
import QtQuick.Window 2.1
import QtQuick.Controls 1.1
import QtQuick.Controls.Private 1.0

Style {
    id: styleRoot

    property string __menuItemType: "menuitem"

    property Component frame: StyleItem {
        elementType: "menu"

        contentWidth: parent ? parent.contentWidth : 0
        contentHeight: parent ? parent.contentHeight : 0
        width: implicitWidth
        height: implicitHeight

        property int subMenuOverlap: -2 * pixelMetric("menupanelwidth")
        property real maxHeight: Screen.desktopAvailableHeight * 0.99
        property int margin: pixelMetric("menuvmargin") + pixelMetric("menupanelwidth")

        Rectangle {
            visible: anchors.margins > 0
            anchors {
                fill: parent
                margins: pixelMetric("menupanelwidth")
            }
            color: __syspal.window
        }

        Accessible.role: Accessible.PopupMenu
    }

    property Component menuItem: StyleItem {
        elementType: __menuItemType
        x: pixelMetric("menuhmargin") + pixelMetric("menupanelwidth")
        y: pixelMetric("menuvmargin")

        text: !!parent && parent.text
        property string textAndShorcut: text + (properties.shortcut ? "\t" + properties.shortcut : "")
        contentWidth: textWidth(textAndShorcut)
        contentHeight: textHeight(textAndShorcut)

        enabled: !!parent && parent.enabled
        selected: !!parent && parent.selected
        on: !!menuItem && !!menuItem["checkable"] && menuItem.checked

        hints: { "showUnderlined": showUnderlined }

        properties: {
            "checkable": !!menuItem && !!menuItem["checkable"],
            "exclusive": !!menuItem && !!menuItem["exclusiveGroup"],
            "shortcut": !!menuItem && menuItem["shortcut"] || "",
            "isSubmenu": isSubmenu,
            "scrollerDirection": scrollerDirection,
            "icon": !!menuItem && menuItem.__icon
        }

        Accessible.role: Accessible.MenuItem
        Accessible.name: StyleHelpers.removeMnemonics(text)
    }

    property Component scrollerStyle: Style {
        padding { left: 0; right: 0; top: 0; bottom: 0 }
        property bool scrollToClickedPosition: false
        property Component frame: Item { visible: false }
        property Component corner: Item { visible: false }
        property Component __scrollbar: Item { visible: false }
        property bool useScrollers: true
    }
}
