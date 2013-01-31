/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

import QtQuick 1.0

Rectangle {
    id: treatsApp
    width: 800
    height: 480
    color: "darkgrey"
    Component.onCompleted: treatsApp.state = "part1"
    signal animationFinished

    Item {
        width: 800
        height: 480
        anchors.centerIn: parent
        clip: true

        Logo {
            id: logo
            x: 165
            y: 35
            rotation: -15
            scale: 0.6
            opacity: 0
            onAnimationFinished: treatsApp.animationFinished();
        }

        Item {
            id: quickblur
            x: 800//325
            y: 344
            Image {
                id: blurText
                source: "quick-blur.png"
            }
            Image {
                id: quickregular
                x: -1
                y: 0
                opacity: 0
                source: "quick-regular.png"
            }
            Image {
                id: star
                x: -1
                y: 0
                opacity: 0
                source: "white-star.png"
                smooth: true
                NumberAnimation on rotation {
                    from: 0
                    to: 360
                    loops: NumberAnimation.Infinite
                    running: true
                    duration: 2000
                }
            }
        }
    }

    states: [
        State {
            name: "part1"
            PropertyChanges {
                target: logo
                scale: 0.8
                opacity: 1
                rotation: 0
            }
            PropertyChanges {
                target: treatsApp
                color: "black"
            }
            PropertyChanges {
                target: logo
                y: 10
            }
            PropertyChanges {
                target: quickblur
                x: logo.x + 145
            }
            PropertyChanges {
                target: blurText
                opacity: 0
            }
            PropertyChanges {
                target: quickregular
                opacity: 1
            }
            PropertyChanges {
                target: star
                x: -7
                y: -37
            }
        }
    ]

    transitions: [
        Transition {
            ParallelAnimation {
                NumberAnimation { target: logo; property: "opacity"; duration: 500 }
                NumberAnimation { target: logo; property: "scale"; duration: 4000; }
                NumberAnimation { target: logo; property: "rotation"; duration: 2000; easing.type: "OutBack"}
                ColorAnimation { duration: 3000}
                SequentialAnimation {
                    PauseAnimation { duration: 1000 }
                    ScriptAction { script: logo.logoState = "showBlueprint" }
                    PauseAnimation { duration: 800 }
                    ScriptAction { script: logo.logoState = "finale" }
                    PauseAnimation { duration: 800 }
                    ParallelAnimation {
                        NumberAnimation { target: quickblur; property: "x"; duration: 200;}
                        SequentialAnimation {
                            PauseAnimation { duration: 200}
                            ParallelAnimation {
                                NumberAnimation { target: blurText; property: "opacity"; duration: 300;}
                                NumberAnimation { target: quickregular; property: "opacity"; duration: 300;}
                            }
                            NumberAnimation { target: star; property: "opacity"; from: 0; to: 1; duration: 500 }
                            PauseAnimation { duration: 200 }
                            NumberAnimation { target: star; property: "opacity"; from: 1; to: 0; duration: 500 }
                        }
                        SequentialAnimation {
                            PauseAnimation { duration: 150}
                            NumberAnimation { target: logo; property: "y"; duration: 300; easing.type: "OutBounce" }
                        }
                    }
                }
            }
        }
    ]

} // treatsApp
