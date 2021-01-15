/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Design Studio.
**
** $QT_BEGIN_LICENSE:BSD$
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
** $QT_END_LICENSE$
**
****************************************************************************/

import QtQuick 2.8

Item {
    id: timedate
    width: 47
    height: 29

    readonly property alias currentHourInt: timeContainer.hrsInt
    readonly property alias currentMinuteInt: timeContainer.minInt

    Text {
        id: timelabel
        x: 2
        y: -1
        width: 43
        height: 16
        color: "#B8B8B8"
        font.pixelSize: 16
        horizontalAlignment: Text.AlignHCenter
        font.family: "Maven Pro"

        text: timeContainer.hrsStr + ":" + timeContainer.minStr
    }


    Item {
        id: timeContainer
        property string hrsStr: "00"
        property string minStr: "00"

        property int hrsInt: 0
        property int minInt: 0

        Timer {
            id: timer
            interval: 1000
            running: true
            repeat: true

            onTriggered: {
                updateTime()
            }

            function updateTime() {
                var currentDate = new Date()
                timeContainer.hrsInt = currentDate.getHours()
                if (timeContainer.hrsInt < 10) timeContainer.hrsStr = "0" + timeContainer.hrsInt
                else timeContainer.hrsStr = timeContainer.hrsInt

                timeContainer.minInt = currentDate.getMinutes()
                if (timeContainer.minInt < 10) timeContainer.minStr = "0" + timeContainer.minInt
                else timeContainer.minStr = timeContainer.minInt
            }
        }
    }

    Component.onCompleted: {
        timer.updateTime()
    }
}
