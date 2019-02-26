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
import QtQuick.Timeline 1.0

Item {
    width: 174
    height: 200

    Item {
        id: rectangleMask
        width: 0
        height: 200
        clip: true

        Image {
            id: image1
            x: 0
            y: 45
            source: "welcome_windows/splash_pngs/Audio_Waves.png"
        }

        Timeline {
            id: timeline
            startFrame: 0
            enabled: true
            endFrame: 2000

            KeyframeGroup {
                target: rectangleMask
                property: "width"

                Keyframe {
                    value: 0
                    frame: 0
                }

                Keyframe {
                    easing.bezierCurve: [0.25, 0.10, 0.25, 1.00, 1, 1]
                    value: 175
                    frame: 997
                }

                Keyframe {
                    easing.bezierCurve: [0.00, 0.00, 0.58, 1.00, 1, 1]
                    value: 0
                    frame: 1998
                }
            }
        }

        PropertyAnimation {
            id: propertyAnimation
            target: timeline
            property: "currentFrame"
            running: true
            loops: -1
            duration: 2000
            from: timeline.startFrame
            to: timeline.endFrame
        }
    }
}
