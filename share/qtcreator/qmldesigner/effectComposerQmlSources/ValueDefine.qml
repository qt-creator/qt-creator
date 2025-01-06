// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import StudioControls as StudioControls

Row {
    id: root

    width: parent.width

    signal valueChanged()

    StudioControls.TextField {
        id: textField

        width: parent.width - 20

        actionIndicatorVisible: false
        translationIndicatorVisible: false

        text: uniformValue

        onEditingFinished: {
            uniformValue = text
            root.valueChanged()
        }
    }
}
