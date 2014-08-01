/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

import QtQuick 2.1
import QtQuick.Controls 1.1 as Controls
import QtQuick.Controls.Styles 1.1

Controls.SpinBox {
    id: spinBox
    property color borderColor: "#222"
    property color highlightColor: "orange"
    property color textColor: colorLogic.textColor
    property variant backendValue;

    ExtendedFunctionButton {
        x: 2
        y: 4
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

    Controls.Slider {
        id: slider

        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.bottom
        height: 10
        visible: false

        maximumValue: spinBox.maximumValue
        minimumValue: spinBox.minimumValue

        value: spinBox.value

        onValueChanged: {
            spinBox.value = value
        }
    }

    style: CustomSpinBoxStyle {
    }
}
