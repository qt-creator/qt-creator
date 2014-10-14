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
** conditions see http://www.qt.io/licensing.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

import QtQuick 2.1
import QtQuick.Controls 1.0

SpinBox {
    id: spinBox
    width: 76

    property color borderColor: "#222"
    property color highlightColor: "orange"
    property color textColor: "#eee"

    function showSlider() {
        timer.stop()
        timer2.start()
        slider.opacity = 1;
    }

    onHoveredChanged: {
        if (hovered)
            showSlider();
    }

    Slider {
        id: slider

        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.bottom
        height: 10;
        //opacity: 0

        maximumValue: spinBox.maximumValue
        minimumValue: spinBox.minimumValue

        value: spinBox.value
        visible: false

        onValueChanged: {
            spinBox.value = value
        }

        Behavior on opacity {
            PropertyAnimation {
                duration: 100
            }
        }

        onHoveredChanged:  {
            if (!hovered) {
                timer.startTimer();
            } else {
                timer2.stop()
                timer.stop()
            }
        }
    }

    Timer {
        id: timer
        repeat: false
        function startTimer() {
            if (!timer.running)
                timer.start()
        }
        interval: 600

        onTriggered: {
            return
            if (!slider.hovered)
                slider.opacity = 0;
        }
    }

    Timer {
        id: timer2
        repeat: false
        interval: 1000
        onTriggered: {
            return
            if (!slider.hovered)
                slider.opacity = 0;
        }
    }

    style: CustomSpinBoxStyle {

    }
}
