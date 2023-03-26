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
    property variant backendValue: backendValues.horizontalAlignment
    property variant value: root.backendValue.enumeration
    property bool baseStateFlag: isBaseState
    property color __currentColor: root.blueHighlight ? StudioTheme.Values.themeIconColorInteraction
                                                      : StudioTheme.Values.themeIconColor

    onValueChanged: {
        buttonAlignLeft.checked = true
        buttonAlignHCenter.checked = false
        buttonAlignRight.checked = false
        buttonAlignJustify.checked = false

        if (root.value !== undefined) {
            if (root.value === "AlignLeft")
                buttonAlignLeft.checked = true
            else if (root.value === "AlignHCenter")
                buttonAlignHCenter.checked = true
            else if (root.value === "AlignRight")
                buttonAlignRight.checked = true
            else if (root.value === "AlignJustify")
                buttonAlignJustify.checked = true
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

        StudioControls.ButtonGroup { id: group }

        StudioControls.AbstractButton {
            id: buttonAlignLeft
            buttonIcon: StudioTheme.Constants.textAlignLeft
            checkable: true
            autoExclusive: true
            StudioControls.ButtonGroup.group: group
            iconColor: root.__currentColor
            onClicked: {
                if (buttonAlignLeft.checked)
                    root.backendValue.setEnumeration(root.scope, "AlignLeft")
            }
        }

        StudioControls.AbstractButton {
            id: buttonAlignHCenter
            buttonIcon: StudioTheme.Constants.textAlignCenter
            checkable: true
            autoExclusive: true
            StudioControls.ButtonGroup.group: group
            iconColor: root.__currentColor
            onClicked: {
                if (buttonAlignHCenter.checked)
                    root.backendValue.setEnumeration(root.scope, "AlignHCenter")
            }
        }

        StudioControls.AbstractButton {
            id: buttonAlignRight
            buttonIcon: StudioTheme.Constants.textAlignRight
            checkable: true
            autoExclusive: true
            StudioControls.ButtonGroup.group: group
            iconColor: root.__currentColor
            onClicked: {
                if (buttonAlignRight.checked)
                    root.backendValue.setEnumeration(root.scope, "AlignRight")
            }
        }

        StudioControls.AbstractButton {
            id: buttonAlignJustify
            buttonIcon: StudioTheme.Constants.textAlignJustified
            checkable: true
            autoExclusive: true
            StudioControls.ButtonGroup.group: group
            iconColor: root.__currentColor
            onClicked: {
                if (buttonAlignRight.checked)
                    root.backendValue.setEnumeration(root.scope, "AlignJustify")
            }
        }
    }
}
