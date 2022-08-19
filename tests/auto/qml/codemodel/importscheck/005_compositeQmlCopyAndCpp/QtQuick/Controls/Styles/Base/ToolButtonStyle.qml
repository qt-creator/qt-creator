// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick 2.1
import QtQuick.Controls 1.1
import QtQuick.Controls.Private 1.0

/*!
    \qmltype ToolButtonStyle
    \internal
    \ingroup controlsstyling
    \inqmlmodule QtQuick.Controls.Styles
*/
Style {
    readonly property ToolButton control: __control
    property Component panel: Item {
        id: styleitem
        implicitWidth: (hasIcon ? 36 : Math.max(label.implicitWidth + frame.border.left + frame.border.right, 36))
                                 + (arrow.visible ? 10 : 0)
        implicitHeight: hasIcon ? 36 : Math.max(label.implicitHeight, 36)

        readonly property bool hasIcon: icon.status === Image.Ready || icon.status === Image.Loading

        Rectangle {
            anchors.fill: parent
            visible: control.pressed || (control.checkable && control.checked)
            color: "lightgray"
            radius:4
            border.color: "#aaa"
        }
        Item {
            anchors.left: parent.left
            anchors.right: arrow.left
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            clip: true
            Text {
                id: label
                visible: !hasIcon
                anchors.centerIn: parent
                text: control.text
            }
            Image {
                id: icon
                anchors.centerIn: parent
                source: control.iconSource
            }
        }

        BorderImage {
            id: frame
            anchors.fill: parent
            anchors.margins: -1
            anchors.topMargin: -2
            anchors.rightMargin: 0
            source: "images/focusframe.png"
            visible: control.activeFocus
            border.left: 4
            border.right: 4
            border.top: 4
            border.bottom: 4
        }

        Image {
            id: arrow
            visible: control.menu !== null
            source: visible ? "images/arrow-down.png" : ""
            anchors.verticalCenter: parent.verticalCenter
            anchors.right: parent.right
            anchors.rightMargin: visible ? 3 : 0
            opacity: control.enabled ? 0.7 : 0.5
        }
    }
}
