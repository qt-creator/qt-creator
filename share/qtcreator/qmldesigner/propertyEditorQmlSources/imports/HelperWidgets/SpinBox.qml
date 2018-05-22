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

Controls.SpinBox {
    id: spinBox

    property color textColor: colorLogic.textColor
    property variant backendValue;

    implicitWidth: 74

    ExtendedFunctionButton {
        x: 4
        anchors.verticalCenter: parent.verticalCenter
        backendValue: spinBox.backendValue
        visible: spinBox.enabled
    }

    ColorLogic {
        id: colorLogic
        backendValue: spinBox.backendValue
        onValueFromBackendChanged: {
            spinBox.value = valueFromBackend;
        }
    }

    property bool hasSlider: false

    height: hasSlider ? 32 : implicitHeight

    onValueChanged: {
        if (backendValue.value !== value)
            backendValue.value = value;
    }

    style: CustomSpinBoxStyle {
    }

}
