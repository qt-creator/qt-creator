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

Item {
    id: wrapper

    property alias decimals: spinBox.decimals
    property alias hasSlider: spinBox.hasSlider

    property real minimumValue
    property real maximumValue
    property real stepSize

    property alias backendValue: spinBox.backendValue

    Component.onCompleted: {
        // TODO minimumValue/maximumValue convertion...
        // TODO step size convertion...
    }

    width: 100
    implicitHeight: spinBox.height

    StudioControls.SpinBox {
        id: spinBox

        property real properValue: value / factor

        from: minimumValue * factor
        to: maximumValue * factor

        width: wrapper.width
        //height: wrapper.height

//        property color textColor: colorLogic.textColor
        property variant backendValue;

        //implicitWidth: 74
/*
        ExtendedFunctionButton {
            x: 4
            anchors.verticalCenter: parent.verticalCenter
            backendValue: spinBox.backendValue
            visible: spinBox.enabled
        }
*/
        /*
        icon: extend.glyph
        Extn2 {
            glyph:
        }
        */

        textColor: colorLogic.textColor

        ColorLogic {
            id: colorLogic
            backendValue: spinBox.backendValue
            onValueFromBackendChanged: {
                spinBox.value = valueFromBackend * factor;
            }
        }

        property bool hasSlider: false

        //height: hasSlider ? 32 : implicitHeight

        onCompressedValueModified: {
            if (backendValue.value !== properValue)
                backendValue.value = properValue;
        }
    /*
        style: CustomSpinBoxStyle {
        }
    */
    }
}
