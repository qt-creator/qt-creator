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
        x: -1
        y: -2
        width: 31
        height: 49
        clip: true

        Item {
            id: rectangle
            x: 0
            y: 0
            width: 200
            height: 200
            clip: true

            Image {
                id: slidersTogether
                scale: 0.5
                source: "welcome_windows/splash_pngs/Sliders_together.png"
            }
        }
    }

    Timeline {
        id: timeline
        startFrame: 0
        enabled: true
        endFrame: 1000

        KeyframeGroup {
            target: rectangleMask
            property: "width"

            Keyframe {
                value: 30
                frame: 0
            }

            Keyframe {
                easing.bezierCurve: [0.55, 0.06, 0.68, 0.19, 1, 1]
                value: 106
                frame: 740
            }

            Keyframe {
                value: 0
                frame: 1000
            }
        }

        KeyframeGroup {
            target: rectangleMask
            property: "height"
            Keyframe {
                value: 49
                frame: 505
            }
        }
    }

    PropertyAnimation {
        id: propertyAnimation
        target: timeline
        property: "currentFrame"
        running: true
        loops: -1
        duration: 1300
        from: timeline.startFrame
        to: timeline.endFrame
    }
}
