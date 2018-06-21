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

ButtonRowButton {
    id: boolButtonRowButton

    property variant backendValue

    property bool isHighlighted: false

    property string standardIconSource
    property string highlightedIconSource

    leftPadding: 18

    iconSource: isHighlighted ? highlightedIconSource : standardIconSource

    QtObject {
        id: innerObject
        function evaluate() {
            if (innerObject.baseStateFlag) {
                if (boolButtonRowButton.backendValue !== null
                        && innerObject.isInModel) {
                    isHighlighted = true
                } else {
                    isHighlighted = false
                }
            } else {
                if (boolButtonRowButton.backendValue !== null
                        && innerObject.isInSubState) {
                    isHighlighted = true
                } else {
                    isHighlighted = false
                }
            }
        }

        property bool baseStateFlag: isBaseState
        onBaseStateFlagChanged: evaluate()

        property bool isInModel: boolButtonRowButton.backendValue.isInModel
        onIsInModelChanged: evaluate()


        property bool isInSubState: boolButtonRowButton.backendValue.isInSubState
        onIsInSubStateChanged: evaluate()

        property variant theValue: boolButtonRowButton.backendValue.value
        onTheValueChanged: {
            evaluate()
            boolButtonRowButton.checked = innerObject.theValue
        }
    }

    onCheckedChanged: {
        boolButtonRowButton.backendValue.value = checked
    }

    ExtendedFunctionButton {
        backendValue: boolButtonRowButton.backendValue
        x: 2
        anchors.verticalCenter: parent.verticalCenter
    }
}
