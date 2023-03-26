// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Controls

DialogButtonBox {
    id: root
    padding: DialogValues.popupDialogPadding
    alignment: Qt.AlignRight | Qt.AlignBottom

    background: Rectangle {
        implicitHeight: 40
        x: 1
        y: 1
        width: parent.width - 2
        height: parent.height - 2
        color: DialogValues.darkPaneColor
    }

    delegate: PopupDialogButton {
        width: root.count === 1 ? root.availableWidth / 2 : undefined
    }
}
