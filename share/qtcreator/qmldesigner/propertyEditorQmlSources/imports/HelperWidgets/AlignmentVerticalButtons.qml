// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.15
import HelperWidgets 2.0
import StudioControls 1.0 as StudioControls
import StudioTheme 1.0 as StudioTheme

Row {
    id: root

    property string scope: "Text"
    property bool blueHighlight: false
    property variant backendValue: backendValues.verticalAlignment
    property variant value: root.backendValue.enumeration
    property bool baseStateFlag: isBaseState
    property color __currentColor: root.blueHighlight ? StudioTheme.Values.themeIconColorInteraction
                                                      : StudioTheme.Values.themeIconColor

    onValueChanged: {
        buttonAlignTop.checked = true
        buttonAlignVCenter.checked = false
        buttonAlignBottom.checked = false

        if (root.value !== undefined) {
            if (root.value === "AlignTop")
                buttonAlignTop.checked = true
            else if (root.value === "AlignVCenter")
                buttonAlignVCenter.checked = true
            else if (root.value === "AlignBottom")
                buttonAlignBottom.checked = true
        }
        root.evaluate()
    }

    property bool isInModel: {
        if (root.backendValue !== undefined && root.backendValue.isInModel !== undefined)
            return root.backendValue.isInModel

        return false
    }
    onIsInModelChanged: root.evaluate()
    property bool isInSubState: {
        if (root.backendValue !== undefined && root.backendValue.isInSubState !== undefined)
            return root.backendValue.isInSubState

        return false
    }
    onIsInSubStateChanged: root.evaluate()

    function evaluate() {
        if (root.baseStateFlag) {
            if (root.backendValue !== null && root.backendValue.isInModel)
                root.blueHighlight = true
            else
                root.blueHighlight = false
        } else {
            if (root.backendValue !== null && root.backendValue.isInSubState)
                root.blueHighlight = true
            else
                root.blueHighlight = false
        }
    }

    ExtendedFunctionLogic {
        id: extFuncLogic
        backendValue: root.backendValue
    }

    StudioControls.ButtonRow {
        id: buttonRow
        actionIndicatorVisible: true

        actionIndicator.icon.color: extFuncLogic.color
        actionIndicator.icon.text: extFuncLogic.glyph
        actionIndicator.onClicked: extFuncLogic.show()
        actionIndicator.forceVisible: extFuncLogic.menuVisible

        StudioControls.ButtonGroup {
            id: group
        }

        StudioControls.AbstractButton {
            id: buttonAlignTop
            buttonIcon: StudioTheme.Constants.textAlignTop
            checkable: true
            autoExclusive: true
            StudioControls.ButtonGroup.group: group
            iconColor: root.__currentColor
            onClicked: {
                if (buttonAlignTop.checked)
                    root.backendValue.setEnumeration(root.scope, "AlignTop")
            }
        }
        StudioControls.AbstractButton {
            id: buttonAlignVCenter
            buttonIcon: StudioTheme.Constants.textAlignMiddle
            checkable: true
            autoExclusive: true
            StudioControls.ButtonGroup.group: group
            iconColor: root.__currentColor
            onClicked: {
                if (buttonAlignVCenter.checked)
                    root.backendValue.setEnumeration(root.scope, "AlignVCenter")
            }
        }
        StudioControls.AbstractButton {
            id: buttonAlignBottom
            buttonIcon: StudioTheme.Constants.textAlignBottom
            checkable: true
            autoExclusive: true
            StudioControls.ButtonGroup.group: group
            iconColor: root.__currentColor
            onClicked: {
                if (buttonAlignBottom.checked)
                    root.backendValue.setEnumeration(root.scope, "AlignBottom")
            }
        }
    }
}
