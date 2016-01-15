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
import "Constants.js" as Constants

Controls.Button {
    property color borderColor: "#222"
    property color highlightColor: "orange"
    property color textColor: "#eee"
    style: ButtonStyle {
        label: Text {
            color: Constants.colorsDefaultText
            anchors.fill: parent
            renderType: Text.NativeRendering
            verticalAlignment: Text.AlignVCenter
            horizontalAlignment: Text.AlignHCenter
            text: control.text
            opacity: enabled ? 1 : 0.7
        }
        background: Rectangle {
            implicitWidth: 100
            implicitHeight: 23
            radius: 3
            gradient: control.pressed ? pressedGradient : gradient
            Gradient{
                id: pressedGradient
                GradientStop{color: "#333" ; position: 0}
            }
            Gradient {
                id: gradient
                GradientStop {color: "#606060" ; position: 0}
                GradientStop {color: "#404040" ; position: 0.07}
                GradientStop {color: "#303030" ; position: 1}
            }
            Rectangle {
                anchors.fill: parent
                anchors.margins: -1
                color: "transparent"
                radius: 4
                opacity: 0.3
                visible: control.activeFocus
            }
        }
    }
}
