// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import StudioControls as StudioControls
import StudioTheme as StudioTheme
import EffectComposerBackend

Row {
    id: itemPane

    width: parent.width
    spacing: 5

    StudioControls.ColorEditor {
        actionIndicatorVisible: false

        // color: uniformValue binding can get overwritten by normal operation of the control
        property color resetValue: uniformValue

        onResetValueChanged: color = uniformValue
        Component.onCompleted: color = uniformValue
        onColorChanged: {
            if (uniformValue !== color)
                uniformValue = color
        }
    }
}
