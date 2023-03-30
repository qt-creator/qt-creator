// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick

ButtonRow {
    id: control

    property alias style: button.style

    property alias buttonIcon: button.buttonIcon
    property alias iconColor: button.iconColor
    property alias iconRotation: button.iconRotation
    property alias checkable: button.checkable
    property alias checked: button.checked

    signal onCheckedChanged()
    signal clicked

    AbstractButton {
        id: button
        onCheckedChanged: control.onCheckedChanged()
        onClicked: control.clicked()
    }
}
