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
import QtQuick.Controls.Styles 1.1
import StudioControls 1.0 as StudioControls
import StudioTheme 1.0 as StudioTheme

Item {
    id: wrapper

    property alias decimals: spinBox.decimals
    property alias hasSlider: spinBox.hasSlider

    property real minimumValue: 0.0
    property real maximumValue: 99
    property real stepSize: 1.0

    property alias backendValue: spinBox.backendValue
    property alias sliderIndicatorVisible: spinBox.sliderIndicatorVisible

    width: 96
    implicitHeight: spinBox.height

    property bool __initialized: false

    Component.onCompleted: {
        wrapper.__initialized = true

        convert("stepSize", stepSize)
        convert("from", minimumValue)
        convert("to", maximumValue)
    }

    onStepSizeChanged: convert("stepSize", stepSize)
    onMinimumValueChanged: convert("from", minimumValue)
    onMaximumValueChanged: convert("to", maximumValue)

    function convert(target, value) {
        if (!wrapper.__initialized)
            return
        spinBox[target] = Math.round(value * spinBox.factor)
    }

    StudioControls.SpinBox {
        id: spinBox

        property real realValue: value / factor
        property variant backendValue
        property bool hasSlider: wrapper.sliderIndicatorVisible

        from: minimumValue * factor
        to: maximumValue * factor
        width: wrapper.width

        ExtendedFunctionLogic {
            id: extFuncLogic
            backendValue: spinBox.backendValue
        }

        actionIndicator.icon.color: extFuncLogic.color
        actionIndicator.icon.text: extFuncLogic.glyph
        actionIndicator.onClicked: extFuncLogic.show()

        ColorLogic {
            id: colorLogic
            backendValue: spinBox.backendValue
            onValueFromBackendChanged: {
                spinBox.value = valueFromBackend * spinBox.factor;
            }
        }

        labelColor: edit ? StudioTheme.Values.themeTextColor : colorLogic.textColor

        onCompressedValueModified: {
            if (backendValue.value !== realValue)
                backendValue.value = realValue;
        }
    }
}
