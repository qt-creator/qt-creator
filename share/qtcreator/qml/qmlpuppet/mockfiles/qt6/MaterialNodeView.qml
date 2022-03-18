/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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

import QtQuick3D 6.0

View3D {
    id: root
    anchors.fill: parent
    environment: sceneEnv

    property Material previewMaterial

    function fitToViewPort()
    {
        // No need to zoom this view, this is here just to avoid runtime warnings
    }

    SceneEnvironment {
        id: sceneEnv
        antialiasingMode: SceneEnvironment.MSAA
        antialiasingQuality: SceneEnvironment.High
    }

    Node {
        DirectionalLight {
            shadowMapQuality: Light.ShadowMapQualityMedium
            shadowFilter: 20
            shadowFactor: 21
            castsShadow: true
            eulerRotation.x: -26
            eulerRotation.y: -57
        }

        PerspectiveCamera {
            y: 125.331
            z: 120
            eulerRotation.x: -31
            clipNear: 1
            clipFar: 1000
        }

        Model {
            id: model
            readonly property bool _edit3dLocked: true // Make this non-pickable

            y: 50
            source: "#Sphere"
            materials: previewMaterial
        }

        Model {
            id: floorModel
            source: "#Rectangle"
            scale.y: 8
            scale.x: 8
            eulerRotation.x: -90
            materials: floorMaterial
            DefaultMaterial {
                id: floorMaterial
                diffuseMap: floorTex

                Texture {
                    id: floorTex
                    source: "../images/floor_tex.png"
                    scaleU: floorModel.scale.x
                    scaleV: floorModel.scale.y
                }
            }
        }
    }
}
