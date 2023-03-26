// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.9
import QtQuick.Templates 2.3
import welcome 1.0
import StudioFonts 1.0

Button {
    id: button

    property color hoverColor: Constants.textHoverColor
    property color defaultColor: Constants.textDefaultColor
    property color checkedColor: Constants.textDefaultColor

    text: "test"

    implicitWidth: background.width
    implicitHeight: background.height

    contentItem: Text {
        id: textButton
        text: button.text

        color: checked ? button.checkedColor :
                         button.hovered ? button.hoverColor :
                                          button.defaultColor
        font.family: StudioFonts.titilliumWeb_regular
        renderType: Text.NativeRendering
        font.pixelSize: 18
    }

    background: Item {
        width: textButton.implicitWidth
        height: textButton.implicitHeight
    }
}
