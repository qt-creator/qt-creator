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

import QtQuick 2.3

Rectangle {
    id: splashBackground
    width: 460
    height: 480
    color: "#11102d"

    layer.enabled: true
    layer.textureSize: Qt.size(width * 2, height * 2)
    layer.smooth: true

    Item {
        id: composition
        width: 460
        height: 480

        layer.enabled: true
        layer.textureSize: Qt.size(width * 2, height * 2)
        layer.smooth: true

        Splash_Image2d_png {
            x: 25
            y: 15
            antialiasing: false
            scale: 1.4
            transform: Matrix4x4 {
                matrix: Qt.matrix4x4(1.12606, 0.06371, 0, 0, 0.26038, 0.90592,
                                     0, 0, 0.00000, 0.0000, 1.0, 0,
                                     0.00121, -0.00009, 0.0, 1)
            }
        }
    }

    Image {
        id: highlight
        x: -56
        y: -19
        width: 520
        height: 506
        fillMode: Image.PreserveAspectFit
        source: "welcome_windows/highlight.png"
    }

    Image {
        id: hand
        x: 245
        y: 227
        width: 224
        height: 264
        visible: true
        fillMode: Image.PreserveAspectFit
        source: "welcome_windows/hand.png"
    }
}
