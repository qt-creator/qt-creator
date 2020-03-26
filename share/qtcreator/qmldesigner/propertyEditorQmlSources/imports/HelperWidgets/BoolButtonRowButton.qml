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

StudioControls.Button {
    id: button

    property variant backendValue
    property bool isHighlighted: false

    iconColor: isHighlighted ? StudioTheme.Values.themeInteraction : StudioTheme.Values.themeTextColor
    actionIndicatorVisible: true
    checkable: true

    QtObject {
        id: innerObject
        function evaluate() {
            if (innerObject.baseStateFlag) {
                if (button.backendValue !== null
                        && innerObject.isInModel) {
                    isHighlighted = true
                } else {
                    isHighlighted = false
                }
            } else {
                if (button.backendValue !== null
                        && innerObject.isInSubState) {
                    isHighlighted = true
                } else {
                    isHighlighted = false
                }
            }
        }

        property bool baseStateFlag: isBaseState
        onBaseStateFlagChanged: evaluate()

        property bool isInModel: button.backendValue.isInModel
        onIsInModelChanged: evaluate()


        property bool isInSubState: button.backendValue.isInSubState
        onIsInSubStateChanged: evaluate()

        property variant theValue: button.backendValue.value
        onTheValueChanged: {
            evaluate()
            button.checked = innerObject.theValue
        }
    }

    onCheckedChanged: {
        button.backendValue.value = button.checked
    }

    ExtendedFunctionLogic {
        id: extFuncLogic
        backendValue: button.backendValue
    }

    actionIndicator.icon.color: extFuncLogic.color
    actionIndicator.icon.text: extFuncLogic.glyph
    actionIndicator.onClicked: extFuncLogic.show()
    actionIndicator.forceVisible: extFuncLogic.menuVisible
}
