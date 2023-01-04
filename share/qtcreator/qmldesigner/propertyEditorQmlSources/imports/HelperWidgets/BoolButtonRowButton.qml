// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.15
import HelperWidgets 2.0
import StudioControls 1.0 as StudioControls
import StudioTheme 1.0 as StudioTheme

StudioControls.Button {
    id: button

    property variant backendValue
    property bool isHighlighted: false

    iconColor: button.isHighlighted ? StudioTheme.Values.themeIconColorInteraction
                                    : StudioTheme.Values.themeIconColor
    actionIndicatorVisible: true
    checkable: true

    QtObject {
        id: innerObject

        function evaluate() {
            if (innerObject.baseStateFlag) {
                if (button.backendValue !== null && innerObject.isInModel)
                    button.isHighlighted = true
                else
                    button.isHighlighted = false
            } else {
                if (button.backendValue !== null && innerObject.isInSubState)
                    button.isHighlighted = true
                else
                    button.isHighlighted = false
            }
        }

        property bool baseStateFlag: isBaseState
        onBaseStateFlagChanged: innerObject.evaluate()

        property bool isInModel: button.backendValue === undefined ? false
                                                                   : button.backendValue.isInModel
        onIsInModelChanged: innerObject.evaluate()

        property bool isInSubState: button.backendValue === undefined ? false
                                                                      : button.backendValue.isInSubState
        onIsInSubStateChanged: innerObject.evaluate()

        property variant theValue: button.backendValue === undefined ? 0 : button.backendValue.value
        onTheValueChanged: {
            innerObject.evaluate()
            button.checked = innerObject.theValue
        }
    }

    onCheckedChanged: button.backendValue.value = button.checked

    ExtendedFunctionLogic {
        id: extFuncLogic
        backendValue: button.backendValue
    }

    actionIndicator.icon.color: extFuncLogic.color
    actionIndicator.icon.text: extFuncLogic.glyph
    actionIndicator.onClicked: extFuncLogic.show()
    actionIndicator.forceVisible: extFuncLogic.menuVisible
}
