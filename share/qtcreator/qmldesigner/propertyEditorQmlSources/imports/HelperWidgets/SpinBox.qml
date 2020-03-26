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
import StudioControls 1.0 as StudioControls
import StudioTheme 1.0 as StudioTheme

Item {
    id: wrapper

    property alias decimals: spinBox.decimals
    property alias hasSlider: spinBox.hasSlider

    property alias minimumValue: spinBox.realFrom
    property alias maximumValue: spinBox.realTo
    property alias stepSize: spinBox.realStepSize

    property alias backendValue: spinBox.backendValue
    property alias sliderIndicatorVisible: spinBox.sliderIndicatorVisible

    property alias realDragRange: spinBox.realDragRange

    width: 96
    implicitHeight: spinBox.height

    onFocusChanged: transaction.end();

    StudioControls.RealSpinBox {
        id: spinBox

        onDragStarted: {
            hideCursor();
            transaction.start();
        }

        onDragEnded: {
            restoreCursor();
            transaction.end();
        }

        onRealValueModified: {
            if (transaction.active())
                commitValue();
        }

        function commitValue() {
            if (spinBox.backendValue.value !== spinBox.realValue)
                spinBox.backendValue.value = spinBox.realValue;
        }

        property variant backendValue
        property bool hasSlider: wrapper.sliderIndicatorVisible

        width: wrapper.width

        ExtendedFunctionLogic {
            id: extFuncLogic
            backendValue: spinBox.backendValue
        }

        actionIndicator.icon.color: extFuncLogic.color
        actionIndicator.icon.text: extFuncLogic.glyph
        actionIndicator.onClicked: extFuncLogic.show()

        actionIndicator.forceVisible: extFuncLogic.menuVisible

        ColorLogic {
            id: colorLogic
            backendValue: spinBox.backendValue
            onValueFromBackendChanged: {
                if (valueFromBackend !== undefined)
                    spinBox.realValue = valueFromBackend
            }
        }

        labelColor: spinBox.edit ? StudioTheme.Values.themeTextColor : colorLogic.textColor

        onCompressedRealValueModified: commitValue()
    }
}
