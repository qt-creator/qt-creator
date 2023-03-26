// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.15
import StudioControls 1.0 as StudioControls

StudioControls.CheckBox {
    id: checkBox

    property variant backendValue
    property string tooltip

    ExtendedFunctionLogic {
        id: extFuncLogic
        backendValue: checkBox.backendValue
    }

    actionIndicator.icon.color: extFuncLogic.color
    actionIndicator.icon.text: extFuncLogic.glyph
    actionIndicator.onClicked: extFuncLogic.show()
    actionIndicator.forceVisible: extFuncLogic.menuVisible

    labelColor: colorLogic.textColor

    ColorLogic {
        id: colorLogic
        backendValue: checkBox.backendValue
        onValueFromBackendChanged: {
            if (colorLogic.valueFromBackend !== undefined
                    && checkBox.checked !== colorLogic.valueFromBackend)
                checkBox.checked = colorLogic.valueFromBackend
        }
    }

    onCheckedChanged: {
        if (backendValue.value !== checkBox.checked)
            backendValue.value = checkBox.checked
    }
}
