// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick 2.1
import QtQuick.Controls 1.1
import QtQuick.Controls.Private 1.0

/*!
    \qmltype TableViewStyle
    \inqmlmodule QtQuick.Controls.Styles
    \since 5.1
    \ingroup viewsstyling
    \brief Provides custom styling for TableView

    Note that this class derives from \l ScrollViewStyle
    and supports all of the properties defined there.
*/
ScrollViewStyle {
    id: root

    /*! The \l TableView attached to this style. */
    readonly property TableView control: __control

    /*! The text color. */
    property color textColor: __syspal.text

    /*! The background color. */
    property color backgroundColor: control.backgroundVisible ? __syspal.base : "transparent"

    /*! The alternate background color. */
    property color alternateBackgroundColor: "#f5f5f5"

    /*! The text highlight color, used within selections. */
    property color highlightedTextColor: "white"

    /*! Activates items on single click. */
    property bool activateItemOnSingleClick: false

    padding.top: control.headerVisible ? 0 : 1

    /*! \qmlproperty Component TableViewStyle::headerDelegate
    Delegate for header. This delegate is described in \l {TableView::headerDelegate}
    */
    property Component headerDelegate: BorderImage {
        height: textItem.implicitHeight * 1.2
        source: "images/header.png"
        border.left: 4
        border.bottom: 2
        border.top: 2
        Text {
            id: textItem
            anchors.fill: parent
            verticalAlignment: Text.AlignVCenter
            horizontalAlignment: Text.AlignLeft
            anchors.leftMargin: 12
            text: styleData.value
            elide: Text.ElideRight
            color: textColor
            renderType: Text.NativeRendering
        }
        Rectangle {
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 1
            anchors.topMargin: 1
            width: 1
            color: "#ccc"
        }
    }

    /*! \qmlproperty Component TableViewStyle::rowDelegate
    Delegate for header. This delegate is described in \l {TableView::rowDelegate}
    */
    property Component rowDelegate: Rectangle {
        height: Math.round(TextSingleton.implicitHeight * 1.2)
        property color selectedColor: styleData.hasActiveFocus ? "#07c" : "#999"
        color: styleData.selected ? selectedColor :
                                    !styleData.alternate ? alternateBackgroundColor : backgroundColor
    }

    /*! \qmlproperty Component TableViewStyle::itemDelegate
    Delegate for item. This delegate is described in \l {TableView::itemDelegate}
    */
    property Component itemDelegate: Item {
        height: Math.max(16, label.implicitHeight)
        property int implicitWidth: label.implicitWidth + 20

        Text {
            id: label
            objectName: "label"
            width: parent.width
            anchors.leftMargin: 12
            anchors.left: parent.left
            anchors.right: parent.right
            horizontalAlignment: styleData.textAlignment
            anchors.verticalCenter: parent.verticalCenter
            anchors.verticalCenterOffset: 1
            elide: styleData.elideMode
            text: styleData.value !== undefined ? styleData.value : ""
            color: styleData.textColor
            renderType: Text.NativeRendering
        }
    }
}

