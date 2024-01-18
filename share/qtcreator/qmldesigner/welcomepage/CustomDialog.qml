// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Controls
import WelcomeScreen 1.0

Dialog {
    id: root
    padding: 12

    background: Rectangle {
        color: Constants.currentDialogBackground
        border.color: Constants.currentDialogBorder
        border.width: 1
    }

    header: Label {
        text: root.title
        visible: root.title
        elide: Label.ElideRight
        font.bold: true
        padding: 12
        color: Constants.currentGlobalText

        background: Rectangle {
            x: 1
            y: 1
            width: parent.width - 2
            height: parent.height - 1
            color: Constants.currentDialogBackground
        }
    }

    footer: CustomDialogButtonBox {
        visible: count > 0
    }

    Overlay.modal: Rectangle {
        color: Color.transparent(Constants.currentDialogBackground, 0.5)
    }
}
