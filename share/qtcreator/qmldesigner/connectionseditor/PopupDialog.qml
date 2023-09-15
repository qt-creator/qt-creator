// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Controls
import HelperWidgets 2.0 as HelperWidgets
import StudioTheme 1.0 as StudioTheme
import ConnectionsEditorEditorBackend

Window {
    id: window

    property alias titleBar: titleBarContent.children
    default property alias content: mainContent.children
    property StudioTheme.ControlStyle style: StudioTheme.Values.controlStyle

    width: 300
    height: column.implicitHeight
    visible: true
    flags: Qt.FramelessWindowHint | Qt.Dialog

        Rectangle {
            anchors.fill: parent
            color: StudioTheme.Values.themePopoutBackground
            border.color: "#636363"//StudioTheme.Values.themeTextColor
        }

    function ensureVerticalPosition() {
        if ((window.y + window.height) > (Screen.height - window.style.dialogScreenMargin)) {
            window.y = (Screen.height - window.height - window.style.dialogScreenMargin)
        }
    }

    onHeightChanged: window.ensureVerticalPosition()

    function popup(item) {
        var padding = 12
        var p = item.mapToGlobal(0, 0)
        window.x = p.x - window.width - padding
        if (window.x < 0)
            window.x = p.x + item.width + padding
        window.y = p.y

        window.ensureVerticalPosition()

        window.show()
        window.raise()
    }

    Column {
        id: column
        anchors.fill: parent

        Item {
            id: titleBarItem
            width: parent.width
            height: StudioTheme.Values.titleBarHeight

            DragHandler {
                id: dragHandler

                target: null
                grabPermissions: PointerHandler.CanTakeOverFromAnything
                onActiveChanged: {
                    if (dragHandler.active)
                        window.startSystemMove() // QTBUG-102488
                }
            }

            Row {
                id: row
                anchors.fill: parent
                anchors.leftMargin: StudioTheme.Values.popupMargin
                anchors.rightMargin: StudioTheme.Values.popupMargin
                spacing: 0

                Item {
                    id: titleBarContent
                    width: row.width - closeIndicator.width //- row.anchors.leftMargin
                    height: row.height
                }

                HelperWidgets.IconIndicator {
                    id: closeIndicator
                    anchors.verticalCenter: parent.verticalCenter
                    icon: StudioTheme.Constants.colorPopupClose
                    pixelSize: StudioTheme.Values.myIconFontSize// * 1.4
                    onClicked: window.close()
                }
            }
        }

        Rectangle {
            width: parent.width - 8
            height: 1
            anchors.horizontalCenter: parent.horizontalCenter
            color: "#636363"
        }

        Item {
            id: mainContent
            width: parent.width - 2 * StudioTheme.Values.popupMargin
            height: mainContent.childrenRect.height + 2 * StudioTheme.Values.popupMargin
            anchors.horizontalCenter: parent.horizontalCenter
        }
    }
}
