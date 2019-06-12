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
import QtQuickDesignerTheme 1.0
import StudioControls 1.0 as StudioControls

Item {
    id: wrapper

    property alias decimals: spinBox.decimals
    property alias hasSlider: spinBox.hasSlider

    property real minimumValue: 0.0
    property real maximumValue: 1.0
    property real stepSize: 0.1

    property alias sliderIndicatorVisible: spinBox.sliderIndicatorVisible

    property real value

    onValueChanged: spinBox.value = wrapper.value * spinBox.factor

    signal compressedValueModified
    signal valueModified

    width: 90
    implicitHeight: spinBox.height

    onStepSizeChanged: spinBox.convert("stepSize", wrapper.stepSize)
    onMinimumValueChanged: spinBox.convert("from", wrapper.minimumValue)
    onMaximumValueChanged: spinBox.convert("to", wrapper.maximumValue)

    StudioControls.SpinBox {
        id: spinBox

        onValueModified: wrapper.valueModified()
        onCompressedValueModified: wrapper.compressedValueModified()

        onValueChanged: {
            if (spinBox.__initialized)
                wrapper.value = spinBox.value / spinBox.factor
        }

        width: wrapper.width
        decimals: 2

        actionIndicatorVisible: false

        property bool __initialized: false

        property bool hasSlider: spinBox.sliderIndicatorVisible

        Component.onCompleted: {
            spinBox.__initialized = true

            spinBox.convert("stepSize", wrapper.stepSize)
            spinBox.convert("from", wrapper.minimumValue)
            spinBox.convert("to", wrapper.maximumValue)

            spinBox.value = wrapper.value * spinBox.factor
        }

        function convert(target, value) {
            if (!spinBox.__initialized)
                return
            spinBox[target] = Math.round(value * spinBox.factor)
        }

    }
}
