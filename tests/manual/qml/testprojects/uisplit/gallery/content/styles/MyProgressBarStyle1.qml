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

import QtQuick 2.2
import QtQuick.Controls 1.2
import QtQuick.Controls.Styles 1.1
import QtQuick.Particles 2.0

ProgressBarStyle {
    background: BorderImage {
        source: "../../images/progress-background.png"
        border.left: 2 ; border.right: 2 ; border.top: 2 ; border.bottom: 2
    }
    progress: Item {
        clip: true
        BorderImage {
            anchors.fill: parent
            anchors.rightMargin: (control.value < control.maximumValue) ? -4 : 0
            source: "../../images/progress-fill.png"
            border.left: 10 ; border.right: 10
            Rectangle {
                width: 1
                color: "#a70"
                opacity: 0.8
                anchors.top: parent.top
                anchors.bottom: parent.bottom
                anchors.bottomMargin: 1
                anchors.right: parent.right
                visible: control.value < control.maximumValue
                anchors.rightMargin: -parent.anchors.rightMargin
            }
        }
        ParticleSystem{ id: bubbles; running: visible }
        ImageParticle{
            id: fireball
            system: bubbles
            source: "../../images/bubble.png"
            opacity: 0.7
        }
        Emitter{
            system: bubbles
            anchors.bottom: parent.bottom
            anchors.margins: 4
            anchors.bottomMargin: -4
            anchors.left: parent.left
            anchors.right: parent.right
            size: 4
            sizeVariation: 4
            acceleration: PointDirection{ y: -6; xVariation: 3 }
            emitRate: 6 * control.value
            lifeSpan: 3000
        }
    }
}
