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

T.Slider {
    id: control

    implicitWidth: Math.max((background ? background.implicitWidth : 0) + leftInset + rightInset,
                            (handle ? handle.implicitWidth : 0) + leftPadding + rightPadding)
    implicitHeight: Math.max((background ? background.implicitHeight : 0) + topInset + bottomInset,
                             (handle ? handle.implicitHeight : 0) + topPadding + bottomPadding)

    leftInset: DefaultStyle.inset - 1
    topInset: DefaultStyle.inset - 3
    rightInset: DefaultStyle.inset - 1
    bottomInset: DefaultStyle.inset - 3
    leftPadding: DefaultStyle.padding - 1
    topPadding: DefaultStyle.padding - 3
    rightPadding: DefaultStyle.padding - 1
    bottomPadding: DefaultStyle.padding - 3

    // Use an intermediate item to simplify the Slider's implicitHeight calculation,
    // since the handle image has extra space in it.
    handle: Item {
        x: (control.orientation == Qt.Horizontal ? 1 : 0) * control.visualPosition * (parent.availableWidth  - handleImage.implicitWidth / 2) + parent.leftInset
        y: (control.orientation == Qt.Vertical ? 1 : 0) * control.visualPosition * (parent.availableHeight  - handleImage.implicitHeight / 2) + parent.topInset
        // 20 is the combined left and right space in the image
        implicitWidth: handleImage.implicitWidth - 20
        // 20 is the combined top and bottom space in the image
        implicitHeight: handleImage.implicitHeight - 20

        Image {
            id: handleImage
            source: control.enabled
                    ? control.pressed
                      ? "images/slider-handle-pressed.png"
                      : "images/slider-handle.png"
                    : control.pressed
                        ? "images/slider-handle-disabled-pressed.png"
                        : "images/slider-handle-disabled.png"
            anchors.verticalCenter: parent.verticalCenter
            anchors.horizontalCenter: parent.horizontalCenter
            // The handle is offset by 5 pixels in the source image.
            anchors.verticalCenterOffset: 5
        }
    }

    background: Item {
        implicitWidth: control.orientation == Qt.Horizontal ? 200 : 3
        implicitHeight: control.orientation == Qt.Horizontal ? 3 : 200

        Item {
            visible: control.orientation == Qt.Horizontal
            anchors.fill: parent

            BorderImage {
                width: parent.width
                height: 9
                anchors.verticalCenter: parent.verticalCenter
                border.left: 4
                border.right: 4
                source: control.enabled
                        ? "images/slider-background.png"
                        : "images/slider-background-disabled.png"
            }

            BorderImage {
                visible: control.enabled && control.visualPosition > 0
                width: handle.x + (handle.implicitWidth / 2)
                height: 9
                anchors.verticalCenter: parent.verticalCenter
                border.left: 4
                border.right: 4
                source: "images/slider-progress.png"
            }
        }

        Item {
            visible: control.orientation == Qt.Vertical
            anchors.fill: parent

            BorderImage {
                height: parent.height
                width: 9
                anchors.horizontalCenter: parent.horizontalCenter
                border.top: 4
                border.bottom: 4
                source: control.enabled
                        ? "images/slider-background-vertical.png"
                        : "images/slider-background-disabled-vertical.png"
            }

            BorderImage {
                visible: control.enabled && control.visualPosition < 1
                height: parent.height - handle.y - (handle.implicitHeight / 2)
                width: 9
                anchors.bottom: parent.bottom
                anchors.horizontalCenter: parent.horizontalCenter
                border.top: 4
                border.bottom: 4
                source: "images/slider-progress-vertical.png"
            }
        }
    }
}
