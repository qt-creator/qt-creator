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

import QtQuick 2.0
import QtQuick3D 1.15
import MouseArea3D 1.0
import LightGeometry 1.0

Node {
    id: lightGizmo

    property View3D view3D
    property Node targetNode: null
    property MouseArea3D dragHelper: null
    property color color: Qt.rgba(1, 1, 0, 1)
    property real brightnessScale: targetNode ? Math.max(1.0, 1.0 + targetNode.brightness) : 100
    property real fadeScale: {
        // Value indicates area where intensity is above certain percent of total brightness.
        if (lightGizmo.targetNode instanceof SpotLight || lightGizmo.targetNode instanceof PointLight) {
            var l = targetNode.linearFade;
            var q = targetNode.quadraticFade;
            var c = targetNode.constantFade;
            var d = 20; // divisor to target intensity value. E.g. 20 = 1/20 = 5%
            if (l === 0 && q === 0)
                l = 1;
            // Solved from equation in shader:
            // 1 / d = 1 / (c + (l + q * dist) * dist);
            if (q === 0)
                return 100 * Math.max(((d - c) / l), 1);
            else
                return 100 * ((Math.sqrt(4 * q * (d - c) + (l * l)) - l) / (2 * q));
        } else {
            return 100;
        }
    }

    position: targetNode ? targetNode.scenePosition : Qt.vector3d(0, 0, 0)
    visible: lightGizmo.targetNode instanceof SpotLight
             || lightGizmo.targetNode instanceof AreaLight
             || lightGizmo.targetNode instanceof DirectionalLight
             || lightGizmo.targetNode instanceof PointLight

    AutoScaleHelper {
        id: autoScale
        view3D: lightGizmo.view3D
    }

    Node {
        rotation: !lightGizmo.targetNode ? Qt.quaternion(1, 0, 0, 0)
                                         : lightGizmo.targetNode.sceneRotation

        LightModel {
            id: spotModel

            property real coneScale: visible
                                     ? lightGizmo.fadeScale * Math.tan(Math.PI * targetNode.coneAngle / 180)
                                     : 1

            geometryName: "Edit 3D SpotLight"
            geometryType: LightGeometry.Spot
            color: lightGizmo.color
            visible: lightGizmo.targetNode instanceof SpotLight
            scale: Qt.vector3d(coneScale, coneScale, lightGizmo.fadeScale)
        }
        Node {
            visible: lightGizmo.targetNode instanceof SpotLight
            SpotLightHandle {
                id: sphereHandle1
                view3D: lightGizmo.view3D
                material: lightMaterial
                position: Qt.vector3d(0, spotModel.scale.y, -spotModel.scale.z)
            }
            SpotLightHandle {
                id: sphereHandle2
                view3D: lightGizmo.view3D
                material: lightMaterial
                position: Qt.vector3d(spotModel.scale.x, 0, -spotModel.scale.z)
            }
            SpotLightHandle {
                id: sphereHandle3
                view3D: lightGizmo.view3D
                material: lightMaterial
                position: Qt.vector3d(0, -spotModel.scale.y, -spotModel.scale.z)
            }
            SpotLightHandle {
                id: sphereHandle4
                view3D: lightGizmo.view3D
                material: lightMaterial
                position: Qt.vector3d(-spotModel.scale.x, 0, -spotModel.scale.z)
            }
        }

        LightModel {
            id: areaModel
            geometryName: "Edit 3D AreaLight"
            geometryType: LightGeometry.Area
            color: lightGizmo.color
            visible: lightGizmo.targetNode instanceof AreaLight
            scale: visible ? Qt.vector3d(lightGizmo.targetNode.width / 2,
                                         lightGizmo.targetNode.height / 2, 1)
                             .times(lightGizmo.targetNode.scale)
                           : Qt.vector3d(1, 1, 1)
        }
        LightModel {
            id: directionalModel
            geometryName: "Edit 3D DirLight"
            geometryType: LightGeometry.Directional
            color: lightGizmo.color
            visible: lightGizmo.targetNode instanceof DirectionalLight
            scale: autoScale.getScale(Qt.vector3d(50, 50, 50))
        }
        LightModel {
            id: pointModel
            geometryName: "Edit 3D PointLight"
            geometryType: LightGeometry.Point
            color: lightGizmo.color
            visible: lightGizmo.targetNode instanceof PointLight
            scale: Qt.vector3d(lightGizmo.fadeScale, lightGizmo.fadeScale, lightGizmo.fadeScale)
        }

        AdjustableArrow {
            id: primaryArrow
            eulerRotation: Qt.vector3d(-90, 0, 0)
            targetNode: lightGizmo.targetNode
            color: lightGizmo.color
            view3D: lightGizmo.view3D
            active: false
            dragHelper: lightGizmo.dragHelper
            scale: autoScale.getScale(Qt.vector3d(5, 5, 5))
            length: lightGizmo.brightnessScale / 10
        }

        DefaultMaterial {
            id: lightMaterial
            emissiveColor: lightGizmo.color
            lighting: DefaultMaterial.NoLighting
            cullMode: Material.NoCulling
        }
    }
}
