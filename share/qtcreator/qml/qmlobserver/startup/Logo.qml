/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtDeclarative module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

import QtQuick 1.0

Rectangle {
    id: myApp
    width: 411
    height: 411
    color: "transparent"
    property alias logoState : myApp.state
    signal animationFinished

    Item {
        id: sketchBlueHolder
        width: sketchLogo.width
        height: sketchLogo.height
        Image {
            id: image1
            x: -44
            y: -45
            smooth: true
            source: "shadow.png"
        }
        Item {
            clip: true
            width: sketchLogo.width
            height: sketchLogo.height
            Image {
                id: sketchLogo
                smooth: true
                source: "qt-sketch.jpg"
            }
            Image {
                id: blueLogo
                y: -420
                smooth: true
                source: "qt-blue.jpg"
            }
        }
    }

    states: [
        State {
            name: "showBlueprint"
            PropertyChanges {
                target: blueLogo
                y: 0
            }
            PropertyChanges {
                target: sketchLogo
                opacity: 0
            }
        },
        State {
            extend: "showBlueprint"
            name: "finale"
            PropertyChanges {
                target: fullLogo
                opacity: 1
            }
            PropertyChanges {
                target: backLogo
                opacity: 1
                scale: 1
            }
            PropertyChanges {
                target: frontLogo
                opacity: 1
                scale: 1
            }
            PropertyChanges {
                target: qtText
                opacity: 1
                scale: 1
            }
            PropertyChanges {
                target: sketchBlueHolder
                opacity: 0
                scale: 1.4
            }
        }
    ]

    transitions: [
        Transition {
            to: "showBlueprint"
            SequentialAnimation {
                NumberAnimation { property: "y"; duration: 600; easing.type: "OutBounce" }
                PropertyAction { target: sketchLogo; property: "opacity" }
            }
        },
        Transition {
            to: "finale"
            PropertyAction { target: fullLogo; property: "opacity" }
            SequentialAnimation {
                NumberAnimation { target: backLogo; properties: "scale, opacity"; duration: 300 }
                NumberAnimation { target: frontLogo; properties: "scale, opacity"; duration: 300 }
                ParallelAnimation {
                    NumberAnimation { target: qtText; properties: "opacity, scale"; duration: 400; easing.type: "OutQuad" }
                    NumberAnimation { target: sketchBlueHolder; property: "opacity"; duration: 300; easing.type: "OutQuad" }
                    NumberAnimation { target: sketchBlueHolder; property: "scale"; duration: 320; easing.type: "OutQuad" }
                }
                PauseAnimation { duration: 1000 }
                ScriptAction { script: myApp.animationFinished() }
            }
        }
    ]

    Item {
        id: fullLogo
        opacity: 0
        Image {
            id: backLogo
            x: -16
            y: -41
            opacity: 0
            scale: 0.7
            smooth: true
            source: "qt-back.png"
        }
        Image {
            id: frontLogo
            x: -17
            y: -41
            opacity: 0
            scale: 1.2
            smooth: true
            source: "qt-front.png"
        }
        Image {
            id: qtText
            x: -10
            y: -41
            opacity: 0
            scale: 1.2
            smooth: true
            source: "qt-text.png"
        }
    }
}
