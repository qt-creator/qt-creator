// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import StudioControls as StudioControls

Item { // The wrapper Item is used to limit hovering and clicking the CheckBox to its area
    id: root
    height: 30

    signal valueChanged()

    StudioControls.CheckBox {
        actionIndicatorVisible: false
        checked: uniformValue
        onToggled: {
            uniformValue = checked
            root.valueChanged()
        }
        anchors.verticalCenter: parent.verticalCenter
    }
}
