/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

import QtQuick 2.1
import HelperWidgets 2.0
import StudioControls 1.0 as StudioControls
import StudioTheme 1.0 as StudioTheme

Row {
    id: alignmentVerticalButtons

    property bool blueHighlight: false

    property variant backendValue: backendValues.verticalAlignment;

    property variant value: backendValue.enumeration

    property bool baseStateFlag: isBaseState;

    property color __currentColor: blueHighlight ? StudioTheme.Values.themeInteraction : StudioTheme.Values.themeTextColor

    onValueChanged: {
        buttonAlignTop.checked = true
        buttonAlignVCenter.checked = false
        buttonAlignBottom.checked = false

        if (value !== undefined) {
            if (value === "AlignTop") {
                buttonAlignTop.checked = true
            } else if (value === "AlignVCenter") {
                buttonAlignVCenter.checked = true
            } else if (value === "AlignBottom") {
                buttonAlignBottom.checked = true
            }
        }
        evaluate()
    }

    property bool isInModel: backendValue.isInModel;
    onIsInModelChanged: {
        evaluate();
    }
    property bool isInSubState: backendValue.isInSubState;
    onIsInSubStateChanged: {
        evaluate();
    }

    function evaluate() {
        if (baseStateFlag) {
            if (backendValue !== null && backendValue.isInModel)
                blueHighlight = true;
            else
                blueHighlight = false;
        } else {
            if (backendValue !== null && backendValue.isInSubState)
                blueHighlight = true;
            else
                blueHighlight = false;
        }
    }

    ExtendedFunctionLogic {
        id: extFuncLogic
        backendValue: alignmentVerticalButtons.backendValue
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
            iconColor: __currentColor
            onClicked: {
                if (checked)
                    backendValue.setEnumeration("Text", "AlignTop")
            }
        }
        StudioControls.AbstractButton {
            id: buttonAlignVCenter
            buttonIcon: StudioTheme.Constants.textAlignMiddle
            checkable: true
            autoExclusive: true
            StudioControls.ButtonGroup.group: group
            iconColor: __currentColor
            onClicked: {
                if (checked)
                    backendValue.setEnumeration("Text", "AlignVCenter")
            }
        }
        StudioControls.AbstractButton {
            id: buttonAlignBottom
            buttonIcon: StudioTheme.Constants.textAlignBottom
            checkable: true
            autoExclusive: true
            StudioControls.ButtonGroup.group: group
            iconColor: __currentColor
            onClicked: {
                if (checked)
                    backendValue.setEnumeration("Text", "AlignBottom")
            }
        }
    }
}
