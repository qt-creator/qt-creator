// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import StudioControls as StudioControls

Row {
    id: root

    signal valueChanged()

    StudioControls.ComboBox {
        model: ["R", "G", "B", "A"]
        actionIndicatorVisible: false
        currentIndex: uniformValue ?? 3
        onActivated: {
            uniformValue = currentIndex
            root.valueChanged()
        }
    }
}
