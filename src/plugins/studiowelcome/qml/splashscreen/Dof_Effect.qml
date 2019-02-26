/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

import QtQuick 2.0
import QtGraphicalEffects 1.0

Item {
    id: root
    default property alias content: stack.children
    property alias maskBlurRadius: maskedBlur.radius
    property alias maskBlurSamples: maskedBlur.samples
    property alias gradientStopPosition1: stop1.position
    property alias gradientStopPosition2: stop2.position
    property alias gradientStopPosition3: stop3.position
    property alias gradientStopPosition4: stop4.position
    property alias maskRotation: gradient.rotation

    Row {
        visible: true
        id: stack
    }

    Item {
        id: mask
        width: stack.width
        height: stack.height
        visible: false

        LinearGradient {
            id: gradient
            height: stack.height * 2
            width: stack.width * 2
            y: -stack.height / 2
            x: -stack.width / 2
            rotation: 0
            gradient: Gradient {
                GradientStop {
                    id: stop1
                    position: 0.2
                    color: "#ffffffff"
                }
                GradientStop {
                    id: stop2
                    position: 0.5
                    color: "#00ffffff"
                }
                GradientStop {
                    id: stop3
                    position: 0.8
                    color: "#00ffffff"
                }
                GradientStop {
                    id: stop4
                    position: 1.0
                    color: "#ffffffff"
                }
            }
            start: Qt.point(stack.width / 2, 0)
            end: Qt.point(stack.width + stack.width / 2, 100)
        }
    }

    MaskedBlur {
        id: maskedBlur
        anchors.fill: stack
        source: stack
        maskSource: mask
        radius: 32
        samples: 16
    }
}
