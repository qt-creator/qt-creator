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

    Image {
        id: image2
        x: 93
        y: 133
        source: "welcome_windows/splash_pngs/ring_1_out.png"

        Image {
            id: image1
            x: 13
            y: 13
            source: "welcome_windows/splash_pngs/ring_1_in.png"
        }
    }

    Image {
        id: image3
        x: 13
        y: 72
        source: "welcome_windows/splash_pngs/ring_2.png"
    }

    Image {
        id: image4
        x: 73
        y: 44
        source: "welcome_windows/splash_pngs/ring_line_2.png"
    }

    Image {
        id: image5
        x: 103
        y: 13
        source: "welcome_windows/splash_pngs/ring_3.png"
    }

    Timeline {
        id: timeline
        startFrame: 0
        enabled: true
        endFrame: 1000

        KeyframeGroup {
            target: image1
            property: "scale"

            Keyframe {
                value: 0.01
                frame: 0
            }

            Keyframe {
                value: 1
                frame: 200
            }
        }

        KeyframeGroup {
            target: image2
            property: "scale"

            Keyframe {
                value: 0.01
                frame: 70
            }

            Keyframe {
                value: 1
                frame: 300
            }
        }

        KeyframeGroup {
            target: image3
            property: "scale"

            Keyframe {
                value: 0.01
                frame: 300
            }

            Keyframe {
                value: 1
                frame: 555
            }
        }

        KeyframeGroup {
            target: image5
            property: "scale"

            Keyframe {
                value: 0.01
                frame: 555
            }

            Keyframe {
                value: 1
                frame: 820
            }
        }
    }

    PropertyAnimation {
        id: propertyAnimation
        target: timeline
        property: "currentFrame"
        running: true
        loops: -1
        duration: 1000
        from: timeline.startFrame
        to: timeline.endFrame
    }

    Image {
        id: image
        x: 46
        y: 105
        source: "welcome_windows/splash_pngs/ring_line_1.png"
    }
}
