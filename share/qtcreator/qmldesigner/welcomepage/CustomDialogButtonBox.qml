// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Controls
import WelcomeScreen 1.0

DialogButtonBox {
    id: root
    padding: 12
    alignment: Qt.AlignRight | Qt.AlignBottom

    background: Rectangle {
        implicitHeight: 40
        x: 1
        y: 1
        width: parent.width - 2
        height: parent.height - 2
        color: Constants.currentDialogBackground
    }

    delegate: DialogButton {
        width: root.count === 1 ? root.availableWidth / 2 : undefined
    }
}
