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
import QtQuick.Controls 1.1 as Controls
import QtQuick.Controls.Styles 1.1

Controls.ComboBox {
    id: comboBox

    property variant backendValue

    property color textColor: colorLogic.textColor
    property string scope: "Qt"

    property bool useInteger: false

    property bool __isCompleted: false

    property bool manualMapping: false

    signal valueFromBackendChanged

    property bool block: false

    ColorLogic {
        id: colorLogic
        backendValue: comboBox.backendValue
        onValueFromBackendChanged: {
            invalidate();
        }

        function invalidate() {

            if (block)
                return

            block = true

            if (manualMapping) {
                valueFromBackendChanged();
            } else if (!comboBox.useInteger) {
                var enumString = comboBox.backendValue.enumeration;

                if (enumString === "")
                    enumString = comboBox.backendValue.value

                var index = comboBox.find(enumString)

                if (index < 0)
                    index = 0

                if (index !== comboBox.currentIndex)
                    comboBox.currentIndex = index

            } else {
                if (comboBox.currentIndex !== backendValue.value)
                    comboBox.currentIndex = backendValue.value
            }

            block = false
        }
    }

    onCurrentTextChanged: {
        if (!__isCompleted)
            return;

        if (backendValue === undefined)
            return;

        if (manualMapping)
            return;

        if (!comboBox.useInteger) {
            backendValue.setEnumeration(comboBox.scope, comboBox.currentText);
        } else {
            backendValue.value = comboBox.currentIndex;
        }
    }

    Component.onCompleted: {
        colorLogic.invalidate()
        __isCompleted = true;
    }

    style: CustomComboBoxStyle {
        textColor: comboBox.textColor
    }

    ExtendedFunctionButton {
        x: 2
        anchors.verticalCenter: parent.verticalCenter
        backendValue: comboBox.backendValue
        visible: comboBox.enabled
    }
}
