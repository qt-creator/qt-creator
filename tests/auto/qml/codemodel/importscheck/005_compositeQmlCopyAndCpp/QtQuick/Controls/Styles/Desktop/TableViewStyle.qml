// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick 2.1
import QtQuick.Controls 1.1
import QtQuick.Controls.Private 1.0
import "."

ScrollViewStyle {
    id: root

    property var __syspal: SystemPalette {
        colorGroup: control.enabled ?
                        SystemPalette.Active : SystemPalette.Disabled
    }
    readonly property TableView control: __control
    property bool activateItemOnSingleClick: __styleitem.styleHint("activateItemOnSingleClick")
    property color textColor: __styleitem.textColor
    property color backgroundColor: __syspal.base
    property color highlightedTextColor: __styleitem.highlightedTextColor

    property StyleItem __styleitem: StyleItem{
        property color textColor: styleHint("textColor")
        property color highlightedTextColor: styleHint("highlightedTextColor")
        elementType: "item"
        visible: false
        active: control.activeFocus
        onActiveChanged: {
            highlightedTextColor = styleHint("highlightedTextColor")
            textColor = styleHint("textColor")
        }
    }

    property Component headerDelegate: StyleItem {
        elementType: "header"
        activeControl: itemSort
        raised: true
        sunken: styleData.pressed
        text: styleData.value
        hover: styleData.containsMouse
        hints: control.styleHints
        properties: {"headerpos": headerPosition}
        property string itemSort:  (control.sortIndicatorVisible && styleData.column === control.sortIndicatorColumn) ? (control.sortIndicatorOrder == Qt.AscendingOrder ? "up" : "down") : "";
        property string headerPosition: control.columnCount === 1 ? "only" :
                                                          styleData.column === control.columnCount-1 ? "end" :
                                                                                  styleData.column === 0 ? "beginning" : ""
    }

    property Component rowDelegate: BorderImage {
        visible: styleData.selected || styleData.alternate
        source: "image://__tablerow/" + (styleData.alternate ? "alternate_" : "")
                + (styleData.selected ? "selected_" : "")
                + (styleData.hasActiveFocus ? "active" : "")
        height: Math.max(16, RowItemSingleton.implicitHeight)
        border.left: 4 ; border.right: 4
    }

    property Component itemDelegate: Item {
        height: Math.max(16, label.implicitHeight)
        property int implicitWidth: label.implicitWidth + 16

        Text {
            id: label
            objectName: "label"
            width: parent.width
            anchors.leftMargin: 8
            font: __styleitem.font
            anchors.left: parent.left
            anchors.right: parent.right
            horizontalAlignment: styleData.textAlignment
            anchors.verticalCenter: parent.verticalCenter
            elide: styleData.elideMode
            text: styleData.value !== undefined ? styleData.value : ""
            color: styleData.textColor
            renderType: Text.NativeRendering
        }
    }
}

