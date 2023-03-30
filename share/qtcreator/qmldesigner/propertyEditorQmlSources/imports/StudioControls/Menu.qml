// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Window
import QtQuick.Templates as T
import StudioTheme 1.0 as StudioTheme

T.Menu {
    id: control

    property StudioTheme.ControlStyle style: StudioTheme.Values.controlStyle

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            contentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             contentHeight + topPadding + bottomPadding)

    font.family: StudioTheme.Constants.font.family
    font.pixelSize: control.style.baseFontSize

    margins: 0
    overlap: 1
    padding: 0

    closePolicy: T.Popup.CloseOnPressOutside | T.Popup.CloseOnPressOutsideParent
                 | T.Popup.CloseOnEscape | T.Popup.CloseOnReleaseOutside
                 | T.Popup.CloseOnReleaseOutsideParent

    delegate: MenuItem { style: control.style }

    contentItem: ListView {
        model: control.contentModel
        interactive: Window.window ? contentHeight > Window.window.height : false
        clip: false
        currentIndex: control.currentIndex
    }

    background: Rectangle {
        implicitWidth: contentItem.childrenRect.width
        implicitHeight: contentItem.childrenRect.height
        color: control.style.popup.background
        border.color: control.style.popup.background
        border.width: control.style.borderWidth

        MouseArea {
            // This mouse area is here to eat clicks that are not handled by menu items
            // to prevent them going through to the underlying view.
            // This is primarily problem with disabled menu items, but right clicks would go
            // through enabled menu items as well.
            anchors.fill: parent
            acceptedButtons: Qt.AllButtons
        }
    }
}
