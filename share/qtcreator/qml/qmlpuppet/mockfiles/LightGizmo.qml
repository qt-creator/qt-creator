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
                l = 1; // For pure constant fade, cone would be infinite, so pretend we have linear fade
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
    readonly property bool dragging: primaryArrow.dragging
                                     || spotLightHandle.dragging
                                     || spotLightInnerHandle.dragging
                                     || spotLightFadeHandle.dragging
                                     || areaHeightHandle.dragging
                                     || areaWidthHandle.dragging
                                     || pointLightFadeHandle.dragging
    property point currentMousePos
    property string currentLabel

    signal propertyValueCommit(string propName)
    signal propertyValueChange(string propName)

    position: targetNode ? targetNode.scenePosition : Qt.vector3d(0, 0, 0)
    visible: lightGizmo.targetNode instanceof SpotLight
             || lightGizmo.targetNode instanceof AreaLight
             || lightGizmo.targetNode instanceof DirectionalLight
             || lightGizmo.targetNode instanceof PointLight

    AutoScaleHelper {
        id: autoScaler
        view3D: lightGizmo.view3D
    }

    Node {
        id: pointLightParts
        rotation: lightGizmo.view3D.camera.rotation
        visible: lightGizmo.targetNode instanceof PointLight

        LightModel {
            id: pointModel
            geometryName: "Edit 3D PointLight"
            geometryType: LightGeometry.Point
            material: lightMaterial
            scale: Qt.vector3d(lightGizmo.fadeScale, lightGizmo.fadeScale, lightGizmo.fadeScale)
        }

        FadeHandle {
            id: pointLightFadeHandle
            view3D: lightGizmo.view3D
            color: (hovering || dragging) ? Qt.rgba(1, 1, 1, 1) : lightGizmo.color
            position: lightGizmo.targetNode instanceof PointLight ? Qt.vector3d(-pointModel.scale.x, 0, 0)
                                                                 : Qt.vector3d(0, 0, 0)
            eulerRotation: Qt.vector3d(0, 0, -90)
            targetNode: lightGizmo.targetNode instanceof PointLight ? lightGizmo.targetNode : null
            active: lightGizmo.targetNode instanceof PointLight
            dragHelper: lightGizmo.dragHelper
            fadeScale: lightGizmo.fadeScale

            onCurrentMousePosChanged: {
                lightGizmo.currentMousePos = currentMousePos;
                lightGizmo.currentLabel = currentLabel;
            }
            onValueChange: lightGizmo.propertyValueChange(propName)
            onValueCommit: lightGizmo.propertyValueCommit(propName)
        }
    }


    Node {
        rotation: !lightGizmo.targetNode ? Qt.quaternion(1, 0, 0, 0)
                                         : lightGizmo.targetNode.sceneRotation

        Node {
            id: spotParts
            visible: lightGizmo.targetNode instanceof SpotLight

            LightModel {
                id: spotModel

                property real coneXYScale: spotParts.visible && lightGizmo.targetNode
                                         ? lightGizmo.fadeScale * Math.tan(Math.PI * lightGizmo.targetNode.coneAngle / 180)
                                         : 1

                geometryName: "Edit 3D SpotLight Cone"
                geometryType: LightGeometry.Spot
                material: lightMaterial
                scale: Qt.vector3d(coneXYScale, coneXYScale,
                                   spotParts.visible && lightGizmo.targetNode && lightGizmo.targetNode.coneAngle > 90
                                   ? -lightGizmo.fadeScale : lightGizmo.fadeScale)
            }

            LightModel {
                id: spotInnerModel

                property real coneXYScale: spotParts.visible && lightGizmo.targetNode
                                         ? lightGizmo.fadeScale * Math.tan(Math.PI * lightGizmo.targetNode.innerConeAngle / 180)
                                         : 1

                geometryName: "Edit 3D SpotLight Inner Cone"
                geometryType: LightGeometry.Spot
                material: lightMaterial
                scale: Qt.vector3d(coneXYScale, coneXYScale,
                                   spotParts.visible && lightGizmo.targetNode && lightGizmo.targetNode.innerConeAngle > 90
                                   ? -lightGizmo.fadeScale : lightGizmo.fadeScale)
            }

            SpotLightHandle {
                id: spotLightHandle
                view3D: lightGizmo.view3D
                color: (hovering || dragging) ? Qt.rgba(1, 1, 1, 1) : lightGizmo.color
                position: lightGizmo.targetNode instanceof SpotLight ? Qt.vector3d(0, spotModel.scale.y, -spotModel.scale.z)
                                                                     : Qt.vector3d(0, 0, 0)
                targetNode: lightGizmo.targetNode instanceof SpotLight ? lightGizmo.targetNode : null
                active: lightGizmo.targetNode instanceof SpotLight
                dragHelper: lightGizmo.dragHelper
                propName: "coneAngle"
                propValue: lightGizmo.targetNode instanceof SpotLight ? targetNode.coneAngle : 0

                onNewValueChanged: targetNode.coneAngle = newValue
                onCurrentMousePosChanged: {
                    lightGizmo.currentMousePos = currentMousePos;
                    lightGizmo.currentLabel = currentLabel;
                }
                onValueChange: lightGizmo.propertyValueChange(propName)
                onValueCommit: {
                    if (targetNode.innerConeAngle > targetNode.coneAngle)
                        targetNode.innerConeAngle = targetNode.coneAngle;
                    lightGizmo.propertyValueCommit(propName)
                    lightGizmo.propertyValueCommit(spotLightInnerHandle.propName);
                }
            }

            SpotLightHandle {
                id: spotLightInnerHandle
                view3D: lightGizmo.view3D
                color: (hovering || dragging) ? Qt.rgba(1, 1, 1, 1) : lightGizmo.color
                position: lightGizmo.targetNode instanceof SpotLight ? Qt.vector3d(0, -spotInnerModel.scale.y, -spotInnerModel.scale.z)
                                                                     : Qt.vector3d(0, 0, 0)
                eulerRotation: Qt.vector3d(180, 0, 0)
                targetNode: lightGizmo.targetNode instanceof SpotLight ? lightGizmo.targetNode : null
                active: lightGizmo.targetNode instanceof SpotLight
                dragHelper: lightGizmo.dragHelper
                propName: "innerConeAngle"
                propValue: lightGizmo.targetNode instanceof SpotLight ? targetNode.innerConeAngle : 0

                onNewValueChanged: targetNode.innerConeAngle = newValue
                onCurrentMousePosChanged: {
                    lightGizmo.currentMousePos = currentMousePos;
                    lightGizmo.currentLabel = currentLabel;
                }
                onValueChange: lightGizmo.propertyValueChange(propName)
                onValueCommit: {
                    if (targetNode.coneAngle < targetNode.innerConeAngle)
                        targetNode.coneAngle = targetNode.innerConeAngle;
                    lightGizmo.propertyValueCommit(propName)
                    lightGizmo.propertyValueCommit(spotLightHandle.propName);
                }
            }

            FadeHandle {
                id: spotLightFadeHandle
                view3D: lightGizmo.view3D
                color: (hovering || dragging) ? Qt.rgba(1, 1, 1, 1) : lightGizmo.color
                position: lightGizmo.targetNode instanceof SpotLight ? Qt.vector3d(spotModel.scale.x / 2, 0, -spotInnerModel.scale.z / 2)
                                                                     : Qt.vector3d(0, 0, 0)
                eulerRotation: Qt.vector3d(90, 0, 0)
                targetNode: lightGizmo.targetNode instanceof SpotLight ? lightGizmo.targetNode : null
                active: lightGizmo.targetNode instanceof SpotLight
                dragHelper: lightGizmo.dragHelper
                fadeScale: lightGizmo.fadeScale
                dragScale: 2

                onCurrentMousePosChanged: {
                    lightGizmo.currentMousePos = currentMousePos;
                    lightGizmo.currentLabel = currentLabel;
                }
                onValueChange: lightGizmo.propertyValueChange(propName)
                onValueCommit: lightGizmo.propertyValueCommit(propName)
            }
        }

        Node {
            id: areaParts
            visible: lightGizmo.targetNode instanceof AreaLight

            LightModel {
                id: areaModel
                geometryName: "Edit 3D AreaLight"
                geometryType: LightGeometry.Area
                material: lightMaterial
                scale: areaParts.visible ? Qt.vector3d(lightGizmo.targetNode.width / 2,
                                                       lightGizmo.targetNode.height / 2, 1)
                                           .times(lightGizmo.targetNode.scale)
                                         : Qt.vector3d(1, 1, 1)
            }

            AreaLightHandle {
                id: areaWidthHandle
                view3D: lightGizmo.view3D
                color: (hovering || dragging) ? Qt.rgba(1, 1, 1, 1) : lightGizmo.color
                position: lightGizmo.targetNode instanceof AreaLight ? Qt.vector3d(-areaModel.scale.x, 0, 0)
                                                                     : Qt.vector3d(0, 0, 0)
                eulerRotation: Qt.vector3d(0, 0, 90)
                targetNode: lightGizmo.targetNode instanceof AreaLight ? lightGizmo.targetNode : null
                active: lightGizmo.targetNode instanceof AreaLight
                dragHelper: lightGizmo.dragHelper
                propName: "width"
                propValue: lightGizmo.targetNode instanceof AreaLight ? targetNode.width : 0

                onNewValueChanged: targetNode.width = newValue
                onCurrentMousePosChanged: {
                    lightGizmo.currentMousePos = currentMousePos;
                    lightGizmo.currentLabel = currentLabel;
                }
                onValueChange: lightGizmo.propertyValueChange(propName)
                onValueCommit: lightGizmo.propertyValueCommit(propName)
            }

            AreaLightHandle {
                id: areaHeightHandle
                view3D: lightGizmo.view3D
                color: (hovering || dragging) ? Qt.rgba(1, 1, 1, 1) : lightGizmo.color
                position: lightGizmo.targetNode instanceof AreaLight ? Qt.vector3d(0, -areaModel.scale.y, 0)
                                                                     : Qt.vector3d(0, 0, 0)
                eulerRotation: Qt.vector3d(0, 0, 180)
                targetNode: lightGizmo.targetNode instanceof AreaLight ? lightGizmo.targetNode : null
                active: lightGizmo.targetNode instanceof AreaLight
                dragHelper: lightGizmo.dragHelper
                propName: "height"
                propValue: lightGizmo.targetNode instanceof AreaLight ? targetNode.height : 0

                onNewValueChanged: targetNode.height = newValue
                onCurrentMousePosChanged: {
                    lightGizmo.currentMousePos = currentMousePos;
                    lightGizmo.currentLabel = currentLabel;
                }
                onValueChange: lightGizmo.propertyValueChange(propName)
                onValueCommit: lightGizmo.propertyValueCommit(propName)
            }
        }

        LightModel {
            id: directionalModel
            geometryName: "Edit 3D DirLight"
            geometryType: LightGeometry.Directional
            material: lightMaterial
            visible: lightGizmo.targetNode instanceof DirectionalLight
            scale: autoScaler.getScale(Qt.vector3d(50, 50, 50))
        }

        AdjustableArrow {
            id: primaryArrow
            eulerRotation: Qt.vector3d(-90, 0, 0)
            targetNode: lightGizmo.targetNode
            color: (hovering || dragging) ? Qt.rgba(1, 1, 1, 1) : lightGizmo.color
            view3D: lightGizmo.view3D
            active: lightGizmo.visible
            dragHelper: lightGizmo.dragHelper
            scale: autoScaler.getScale(Qt.vector3d(5, 5, 5))
            length: (lightGizmo.brightnessScale / 10) + 3

            property real _startBrightness

            function updateBrightness(relativeDistance, screenPos)
            {
                var currentValue = Math.round(Math.max(0, _startBrightness + relativeDistance * 10));
                var l = Qt.locale();
                lightGizmo.currentLabel = "brightness" + qsTr(": ") + Number(currentValue).toLocaleString(l, 'f', 0);
                lightGizmo.currentMousePos = screenPos;
                targetNode.brightness = currentValue;
            }

            onPressed: {
                _startBrightness = targetNode.brightness;
                updateBrightness(0, screenPos);
            }

            onDragged: {
                updateBrightness(relativeDistance, screenPos);
                lightGizmo.propertyValueChange("brightness");
            }

            onReleased: {
                updateBrightness(relativeDistance, screenPos);
                lightGizmo.propertyValueCommit("brightness");
            }
        }

        DefaultMaterial {
            id: lightMaterial
            emissiveColor: lightGizmo.color
            lighting: DefaultMaterial.NoLighting
            cullMode: Material.NoCulling
        }
    }
}
