/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Design Studio.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
**
****************************************************************************/

import QtQuick 2.15
import QtQuick.Templates 2.15 as T

T.Dial {
    id: control

    implicitWidth: Math.max((background ? background.implicitWidth : 0) + leftInset + rightInset,
                            (contentItem ? contentItem.implicitWidth : 0) + leftPadding + rightPadding)
    implicitHeight: Math.max((background ? background.implicitHeight : 0) + topInset + bottomInset,
                             (contentItem ? contentItem.implicitHeight : 0) + topPadding + bottomPadding)

    leftPadding: DefaultStyle.padding
    rightPadding: DefaultStyle.padding
    topPadding: DefaultStyle.padding
    bottomPadding: DefaultStyle.padding

    leftInset: DefaultStyle.inset
    topInset: DefaultStyle.inset
    rightInset: DefaultStyle.inset
    bottomInset: DefaultStyle.inset

    handle: Image {
        id: handleImg
        // Start from the center of the dial, while also accounting for
        // our size (21x15; i.e. the distance from the center
        // of the image to the top of the handle in slider-handle.png).
        // Then move out to the edge of the dial (95).
        x: (control.width / 2) - 21 + (95 * Math.cos(angleRadians))
        y: (control.height / 2) - 15 + (95 * Math.sin(angleRadians))
        source: "images/slider-handle.png"
        readonly property real angleRadians: (control.angle - 90) * (Math.PI / 180)
    }

    background: Image {
        id: backgroundImg
        x: (parent.width - width) / 2
        y: (parent.height - height) / 2
        source: !control.enabled
                ? "images/dial-background-disabled.png"
                : "images/dial-background.png"

        Repeater {
            model: 4 // One for each quadrant

            Item {
                clip: true;
                visible: control.enabled
                width: {
                    var quadrantSize = backgroundImg.width/2;
                    if (control.angle < -180 + index*90) return 0;
                    if (control.angle > -90 + index*90) return quadrantSize;
                    var low = control.angle + 180 - index*90 < 45
                    var handlePos = (handleImg.x - backgroundImg.x + handleImg.width/2);
                    switch (index) {
                    case 0: return low ? quadrantSize - handlePos : quadrantSize;
                    case 1: return low ? quadrantSize : handlePos;
                    case 2: return low ? handlePos - quadrantSize : quadrantSize;
                    case 3: return low ? quadrantSize : quadrantSize*2 - handlePos;
                    }
                }
                x: {
                    switch (index) {
                    case 0: return backgroundImg.width/2 - width;
                    case 1: return 0;
                    case 2: return backgroundImg.width/2;
                    case 3: return backgroundImg.width - width;
                    }
                }

                height: {
                    var quadrantSize = backgroundImg.height/2;
                    if (control.angle < -180 + index*90) return 0;
                    if (control.angle > -90 + index*90) return quadrantSize;
                    var low = control.angle + 180 - index*90 < 45
                    var handlePos = (handleImg.y - backgroundImg.y + handleImg.height/2);
                    switch (index) {
                    case 0: return low ? quadrantSize : quadrantSize*2 - handlePos;
                    case 1: return low ? quadrantSize - handlePos : quadrantSize;
                    case 2: return low ? quadrantSize : handlePos;
                    case 3: return low ? handlePos - quadrantSize : quadrantSize;
                    }
                }
                y: {
                    switch (index) {
                    case 0: return backgroundImg.height - height;
                    case 1: return backgroundImg.height /2 - height;
                    case 2: return 0;
                    case 3: return backgroundImg.height /2;
                    }
                }

                Image {
                    source: "images/dial-progress.png"
                    x: -parent.x
                    y: -parent.y
                    width: backgroundImg.width
                    height: backgroundImg.height
                }
            }
        }
    }
}
