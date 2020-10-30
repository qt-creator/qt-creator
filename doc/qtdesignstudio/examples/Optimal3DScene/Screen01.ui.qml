/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
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

import QtQuick 2.12
import QtQuick3D 1.15
import Optimal3DScene 1.0
import Quick3DAssets.High 1.0
import Quick3DAssets.Low 1.0
import Quick3DAssets.Merged 1.0
import QtQuick 2.15
import Quick3DAssets.VertexColor 1.0

Rectangle {
    width: Constants.width
    height: Constants.height

    color: Constants.backgroundColor
    property alias high: high

    View3D {
        id: view3D
        x: 0
        y: 0
        width: 1280
        height: 720
        SceneEnvironment {
            id: sceneEnvironment
            antialiasingQuality: SceneEnvironment.High
            antialiasingMode: SceneEnvironment.MSAA
        }

        Node {
            id: scenelow

            PerspectiveCamera {
                id: camera
                x: 0
                y: 501.999
                z: 953.07697
            }

            Low {
                id: low
                eulerRotation.z: 0.00001
                eulerRotation.y: -135
                eulerRotation.x: 0.00002
            }

            PointLight {
                id: lightPoint
                x: 0
                y: 888.433
                castsShadow: true
                brightness: 400
                quadraticFade: 0.01318
                z: -0.00007
            }
        }

        Node {
            id: scenehigh
            PerspectiveCamera {
                id: camera1
                x: 0
                y: 501.999
                z: 953.07697
            }

            PointLight {
                id: lightPoint1
                x: 0
                y: 888.433
                brightness: 400
                castsShadow: true
                z: -0.00007
                quadraticFade: 0.01318
            }

            High {
                id: high
                eulerRotation.y: -135
            }
        }

        Node {
            id: scenecombined
            PerspectiveCamera {
                id: camera2
                x: 0
                y: 501.999
                z: 953.07697
            }

            PointLight {
                id: lightPoint2
                x: 0
                y: 888.433
                brightness: 350
                castsShadow: true
                z: -0.00007
                quadraticFade: 0.01318
            }

            Merged {
                id: merged
                eulerRotation.y: -135
            }
        }

        Node {
            id: scenevertexcolor
            PerspectiveCamera {
                id: camera3
                x: 0
                y: 501.999
                z: 953.07697
            }

            PointLight {
                id: lightPoint3
                x: 0
                y: 888.433
                brightness: 350
                castsShadow: true
                z: -0.00007
                quadraticFade: 0.01318
            }

            VertexColor {
                id: vertexColor
                eulerRotation.y: -135
            }
        }
        environment: sceneEnvironment
    }
}

/*##^##
Designer {
    D{i:0;active3dScene:10;formeditorZoom:0.6600000262260437}D{i:3;invisible:true}D{i:7;invisible:true}
D{i:11;invisible:true}
}
##^##*/

