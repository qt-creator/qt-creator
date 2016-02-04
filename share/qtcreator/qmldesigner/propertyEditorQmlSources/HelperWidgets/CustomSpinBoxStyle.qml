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

SpinBoxStyle {

    selectionColor: spinBox.textColor
    selectedTextColor: "black"
    textColor: spinBox.textColor
    padding.top: 3
    padding.bottom: 1
    padding.right: 18
    padding.left: 12

    incrementControl: Item {
        implicitWidth: 14
        implicitHeight: parent.height/2
        opacity: styleData.upEnabled ? styleData.upPressed ? 0.5 : 1 : 0.5
        Image {
            source: "images/up-arrow.png"
            x: 1
            y: 5
        }
    }

    decrementControl: Item {
        implicitWidth: 14
        implicitHeight: parent.height/2
        opacity: styleData.downEnabled ? styleData.downPressed ? 0.5 : 1 : 0.5
        Image {
            source: "images/down-arrow.png"
            x: 1
            y: 2.5 // Hack! Spinbox sets an non-int offset, somewhere
        }
    }

    background: Rectangle {
        implicitWidth: Math.max(64, styleData.contentWidth)
        implicitHeight: 23
        border.color: borderColor
        gradient: Gradient {
            GradientStop {color: "#2c2c2c" ; position: 0}
            GradientStop {color: "#343434" ; position: 0.15}
            GradientStop {color: "#373737" ; position: 1}
        }
        Rectangle {
            border.color: highlightColor
            anchors.fill: parent
            anchors.margins: -1
            color: "transparent"
            opacity: 0.3
            visible: control.activeFocus
        }
    }
}
