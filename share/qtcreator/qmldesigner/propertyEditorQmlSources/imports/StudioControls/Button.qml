// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.15
import StudioControls 1.0

ButtonRow {
    id: myButtonRow

    property alias buttonIcon: myAbstractButton.buttonIcon
    property alias iconColor: myAbstractButton.iconColor
    property alias iconRotation: myAbstractButton.iconRotation
    property alias checkable: myAbstractButton.checkable
    property alias checked: myAbstractButton.checked

    signal onCheckedChanged()
    signal clicked

    AbstractButton {
        id: myAbstractButton
        onCheckedChanged: myButtonRow.onCheckedChanged()
        onClicked: myButtonRow.clicked()
    }
}
