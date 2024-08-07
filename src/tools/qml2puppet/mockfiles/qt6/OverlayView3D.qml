// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick3D
import MouseArea3D

View3D {
    id: overlayView

    property alias moveGizmo: moveGizmo
    property alias rotateGizmo: rotateGizmo
    property alias scaleGizmo: scaleGizmo
    property alias lightGizmo: lightGizmo
    property alias lookAtGizmo: lookAtGizmo

    property var viewRoot: null
    property View3D editView: null
    property bool isActive: viewRoot.overlayViews[viewRoot.activeSplit] === overlayView

    property var lightIconGizmos: []
    property var cameraGizmos: []
    property var particleSystemIconGizmos: []
    property var particleEmitterGizmos: []
    property var reflectionProbeGizmos: []

    signal commitObjectProperty(var objects, var propNames)
    signal changeObjectProperty(var objects, var propNames)

    camera: viewRoot.usePerspective ? overlayPerspectiveCamera : overlayOrthoCamera

    anchors.fill: parent
    z: 2
    clip: true

    environment: sceneEnv

    function addLightGizmo(scene, obj)
    {
        // Insert into first available gizmo if we don't already have gizmo for this object
        var slotFound = -1;
        for (var i = 0; i < lightIconGizmos.length; ++i) {
            if (!lightIconGizmos[i].targetNode) {
                slotFound = i;
            } else if (lightIconGizmos[i].targetNode === obj) {
                lightIconGizmos[i].scene = scene;
                return;
            }
        }

        if (slotFound !== -1) {
            lightIconGizmos[slotFound].scene = scene;
            lightIconGizmos[slotFound].targetNode = obj;
            lightIconGizmos[slotFound].locked = _generalHelper.isLocked(obj);
            lightIconGizmos[slotFound].hidden = _generalHelper.isHidden(obj);
            return;
        }

        // No free gizmos available, create a new one
        var gizmoComponent = Qt.createComponent("LightIconGizmo.qml");
        if (gizmoComponent.status === Component.Ready) {
            var gizmo = gizmoComponent.createObject(overlayView,
                                                    {"view3D": overlayView,
                                                     "targetNode": obj,
                                                     "selectedNodes": viewRoot.selectedNodes,
                                                     "scene": scene,
                                                     "activeScene": viewRoot.activeScene,
                                                     "locked": _generalHelper.isLocked(obj),
                                                     "hidden": _generalHelper.isHidden(obj),
                                                     "globalShow": viewRoot.showIconGizmo});
            lightIconGizmos[lightIconGizmos.length] = gizmo;
            gizmo.clicked.connect(viewRoot.handleObjectClicked);
            gizmo.selectedNodes = Qt.binding(function() {return viewRoot.selectedNodes;});
            gizmo.activeScene = Qt.binding(function() {return viewRoot.activeScene;});
            gizmo.globalShow = Qt.binding(function() {return viewRoot.showIconGizmo;});
        }
    }

    function releaseLightGizmo(obj)
    {
        for (var i = 0; i < lightIconGizmos.length; ++i) {
            if (lightIconGizmos[i].targetNode === obj) {
                lightIconGizmos[i].scene = null;
                lightIconGizmos[i].targetNode = null;
                return;
            }
        }
    }

    function updateLightGizmoScene(scene, obj)
    {
        for (var i = 0; i < lightIconGizmos.length; ++i) {
            if (lightIconGizmos[i].targetNode === obj) {
                lightIconGizmos[i].scene = scene;
                return;
            }
        }
    }

    function addCameraGizmo(scene, obj)
    {
        // Insert into first available gizmo if we don't already have gizmo for this object
        var slotFound = -1;
        for (var i = 0; i < cameraGizmos.length; ++i) {
            if (!cameraGizmos[i].targetNode) {
                slotFound = i;
            } else if (cameraGizmos[i].targetNode === obj) {
                cameraGizmos[i].scene = scene;
                return;
            }
        }

        if (slotFound !== -1) {
            cameraGizmos[slotFound].scene = scene;
            cameraGizmos[slotFound].targetNode = obj;
            cameraGizmos[slotFound].locked = _generalHelper.isLocked(obj);
            cameraGizmos[slotFound].hidden = _generalHelper.isHidden(obj);
            return;
        }

        // No free gizmos available, create a new one
        var gizmoComponent = Qt.createComponent("CameraGizmo.qml");
        var frustumComponent = Qt.createComponent("CameraFrustum.qml");
        if (gizmoComponent.status === Component.Ready && frustumComponent.status === Component.Ready) {
            var geometryName = _generalHelper.generateUniqueName("CameraGeometry");
            var frustum = frustumComponent.createObject(
                        sceneNode,
                        {"geometryName": geometryName, "viewPortRect": viewRoot.viewPortRect});
            var gizmo = gizmoComponent.createObject(
                        overlayView,
                        {"view3D": overlayView, "targetNode": obj,
                         "selectedNodes": viewRoot.selectedNodes, "scene": scene, "activeScene": viewRoot.activeScene,
                         "locked": _generalHelper.isLocked(obj), "hidden": _generalHelper.isHidden(obj),
                         "globalShow": viewRoot.showIconGizmo, "globalShowFrustum": viewRoot.showCameraFrustum});

            cameraGizmos[cameraGizmos.length] = gizmo;
            gizmo.clicked.connect(viewRoot.handleObjectClicked);
            gizmo.selectedNodes = Qt.binding(function() {return viewRoot.selectedNodes;});
            gizmo.activeScene = Qt.binding(function() {return viewRoot.activeScene;});
            gizmo.globalShow = Qt.binding(function() {return viewRoot.showIconGizmo;});
            gizmo.globalShowFrustum = Qt.binding(function() {return viewRoot.showCameraFrustum;});
            frustum.viewPortRect = Qt.binding(function() {return viewRoot.viewPortRect;});
            gizmo.connectFrustum(frustum);
        }
    }

    function releaseCameraGizmo(obj)
    {
        for (var i = 0; i < cameraGizmos.length; ++i) {
            if (cameraGizmos[i].targetNode === obj) {
                cameraGizmos[i].scene = null;
                cameraGizmos[i].targetNode = null;
                return;
            }
        }
    }

    function updateCameraGizmoScene(scene, obj)
    {
        for (var i = 0; i < cameraGizmos.length; ++i) {
            if (cameraGizmos[i].targetNode === obj) {
                cameraGizmos[i].scene = scene;
                return;
            }
        }
    }

    function addParticleSystemGizmo(scene, obj)
    {
        // Insert into first available gizmo if we don't already have gizmo for this object
        var slotFound = -1;
        for (var i = 0; i < particleSystemIconGizmos.length; ++i) {
            if (!particleSystemIconGizmos[i].targetNode) {
                slotFound = i;
            } else if (particleSystemIconGizmos[i].targetNode === obj) {
                particleSystemIconGizmos[i].scene = scene;
                return;
            }
        }

        if (slotFound !== -1) {
            particleSystemIconGizmos[slotFound].scene = scene;
            particleSystemIconGizmos[slotFound].targetNode = obj;
            particleSystemIconGizmos[slotFound].locked = _generalHelper.isLocked(obj);
            particleSystemIconGizmos[slotFound].hidden = _generalHelper.isHidden(obj);
            return;
        }

        // No free gizmos available, create a new one
        var gizmoComponent = Qt.createComponent("ParticleSystemGizmo.qml");
        if (gizmoComponent.status === Component.Ready) {
            var gizmo = gizmoComponent.createObject(overlayView,
                                                    {"view3D": overlayView,
                                                     "targetNode": obj,
                                                     "selectedNodes": viewRoot.selectedNodes,
                                                     "scene": scene,
                                                     "activeScene": viewRoot.activeScene,
                                                     "locked": _generalHelper.isLocked(obj),
                                                     "hidden": _generalHelper.isHidden(obj),
                                                     "globalShow": viewRoot.showIconGizmo,
                                                     "activeParticleSystem": viewRoot.activeParticleSystem});
            particleSystemIconGizmos[particleSystemIconGizmos.length] = gizmo;
            gizmo.clicked.connect(viewRoot.handleObjectClicked);
            gizmo.selectedNodes = Qt.binding(function() {return viewRoot.selectedNodes;});
            gizmo.activeScene = Qt.binding(function() {return viewRoot.activeScene;});
            gizmo.globalShow = Qt.binding(function() {return viewRoot.showIconGizmo;});
            gizmo.activeParticleSystem = Qt.binding(function() {return viewRoot.activeParticleSystem;});
        }
    }

    function releaseParticleSystemGizmo(obj)
    {
        for (var i = 0; i < particleSystemIconGizmos.length; ++i) {
            if (particleSystemIconGizmos[i].targetNode === obj) {
                particleSystemIconGizmos[i].scene = null;
                particleSystemIconGizmos[i].targetNode = null;
                return;
            }
        }
    }

    function updateParticleSystemGizmoScene(scene, obj)
    {
        for (var i = 0; i < particleSystemIconGizmos.length; ++i) {
            if (particleSystemIconGizmos[i].targetNode === obj) {
                particleSystemIconGizmos[i].scene = scene;
                return;
            }
        }
    }

    function addParticleEmitterGizmo(scene, obj)
    {
        // Insert into first available gizmo if we don't already have gizmo for this object
        var slotFound = -1;
        for (var i = 0; i < particleEmitterGizmos.length; ++i) {
            if (!particleEmitterGizmos[i].targetNode) {
                slotFound = i;
            } else if (particleEmitterGizmos[i].targetNode === obj) {
                particleEmitterGizmos[i].scene = scene;
                return;
            }
        }

        if (slotFound !== -1) {
            particleEmitterGizmos[slotFound].scene = scene;
            particleEmitterGizmos[slotFound].targetNode = obj;
            particleEmitterGizmos[slotFound].hidden = _generalHelper.isHidden(obj);
            particleEmitterGizmos[slotFound].systemHidden = _generalHelper.isHidden(obj.system);
            return;
        }

        // No free gizmos available, create a new one
        var gizmoComponent = Qt.createComponent("ParticleEmitterGizmo.qml");
        if (gizmoComponent.status === Component.Ready) {
            var gizmo = gizmoComponent.createObject(
                        overlayScene,
                        {"view3d": overlayView, "targetNode": obj, "selectedNodes": viewRoot.selectedNodes,
                         "activeParticleSystem": viewRoot.activeParticleSystem, "scene": scene,
                         "activeScene": viewRoot.activeScene, "hidden": _generalHelper.isHidden(obj),
                         "systemHidden": _generalHelper.isHidden(obj.system),
                         "globalShow": viewRoot.showParticleEmitter});

            particleEmitterGizmos[particleEmitterGizmos.length] = gizmo;
            gizmo.selectedNodes = Qt.binding(function() {return viewRoot.selectedNodes;});
            gizmo.activeParticleSystem = Qt.binding(function() {return viewRoot.activeParticleSystem;});
            gizmo.globalShow = Qt.binding(function() {return viewRoot.showParticleEmitter;});
            gizmo.activeScene = Qt.binding(function() {return viewRoot.activeScene;});
        }
    }

    function releaseParticleEmitterGizmo(obj)
    {
        for (var i = 0; i < particleEmitterGizmos.length; ++i) {
            if (particleEmitterGizmos[i].targetNode === obj) {
                particleEmitterGizmos[i].scene = null;
                particleEmitterGizmos[i].targetNode = null;
                return;
            }
        }
    }

    function updateParticleEmitterGizmoScene(scene, obj)
    {
        for (var i = 0; i < particleEmitterGizmos.length; ++i) {
            if (particleEmitterGizmos[i].targetNode === obj) {
                particleEmitterGizmos[i].scene = scene;
                return;
            }
        }
    }

    function addReflectionProbeGizmo(scene, obj)
    {
        // Insert into first available gizmo if we don't already have gizmo for this object
        var slotFound = -1;
        for (var i = 0; i < reflectionProbeGizmos.length; ++i) {
            if (!reflectionProbeGizmos[i].targetNode) {
                slotFound = i;
            } else if (reflectionProbeGizmos[i].targetNode === obj) {
                reflectionProbeGizmos[i].scene = scene;
                return;
            }
        }

        if (slotFound !== -1) {
            reflectionProbeGizmos[slotFound].scene = scene;
            reflectionProbeGizmos[slotFound].targetNode = obj;
            reflectionProbeGizmos[slotFound].locked = _generalHelper.isLocked(obj);
            reflectionProbeGizmos[slotFound].hidden = _generalHelper.isHidden(obj);
            return;
        }

        // No free gizmos available, create a new one
        var gizmoComponent = Qt.createComponent("ReflectionProbeGizmo.qml");
        if (gizmoComponent.status === Component.Ready) {
            var gizmo = gizmoComponent.createObject(overlayView,
                                                    {"view3D": overlayView,
                                                     "targetNode": obj,
                                                     "selectedNodes": viewRoot.selectedNodes,
                                                     "scene": scene,
                                                     "activeScene": viewRoot.activeScene,
                                                     "locked": _generalHelper.isLocked(obj),
                                                     "hidden": _generalHelper.isHidden(obj),
                                                     "globalShow": viewRoot.showIconGizmo});
            reflectionProbeGizmos[reflectionProbeGizmos.length] = gizmo;
            gizmo.clicked.connect(viewRoot.handleObjectClicked);
            gizmo.selectedNodes = Qt.binding(function() {return viewRoot.selectedNodes;});
            gizmo.activeScene = Qt.binding(function() {return viewRoot.activeScene;});
            gizmo.globalShow = Qt.binding(function() {return viewRoot.showIconGizmo;});
        }
    }

    function releaseReflectionProbeGizmo(obj)
    {
        for (var i = 0; i < reflectionProbeGizmos.length; ++i) {
            if (reflectionProbeGizmos[i].targetNode === obj) {
                reflectionProbeGizmos[i].scene = null;
                reflectionProbeGizmos[i].targetNode = null;
                return;
            }
        }
    }

    function updateReflectionProbeGizmoScene(scene, obj)
    {
        for (var i = 0; i < reflectionProbeGizmos.length; ++i) {
            if (reflectionProbeGizmos[i].targetNode === obj) {
                reflectionProbeGizmos[i].scene = scene;
                return;
            }
        }
    }

    function gizmoAt(x, y)
    {
        let i = 0;
        for (; i < lightIconGizmos.length; ++i) {
            if (lightIconGizmos[i].visible && lightIconGizmos[i].hasPoint(x, y))
                return lightIconGizmos[i].targetNode;
        }
        for (i = 0; i < cameraGizmos.length; ++i) {
            if (cameraGizmos[i].visible && cameraGizmos[i].hasPoint(x, y))
                return cameraGizmos[i].targetNode;
        }
        for (i = 0; i < particleSystemIconGizmos.length; ++i) {
            if (particleSystemIconGizmos[i].visible && particleSystemIconGizmos[i].hasPoint(x, y))
                return particleSystemIconGizmos[i].targetNode;
        }
        for (i = 0; i < reflectionProbeGizmos.length; ++i) {
            if (reflectionProbeGizmos[i].visible && reflectionProbeGizmos[i].hasPoint(x, y))
                return reflectionProbeGizmos[i].targetNode;
        }
        return null;
    }

    function handleLockedStateChange(node)
    {
        let i = 0;
        for (; i < lightIconGizmos.length; ++i) {
            if (lightIconGizmos[i].targetNode === node) {
                lightIconGizmos[i].locked = _generalHelper.isLocked(node);
                return;
            }
        }
        for (i = 0; i < cameraGizmos.length; ++i) {
            if (cameraGizmos[i].targetNode === node) {
                cameraGizmos[i].locked = _generalHelper.isLocked(node);
                return;
            }
        }
        for (i = 0; i < particleSystemIconGizmos.length; ++i) {
            if (particleSystemIconGizmos[i].targetNode === node) {
                particleSystemIconGizmos[i].locked = _generalHelper.isLocked(node);
                return;
            }
        }
        for (i = 0; i < reflectionProbeGizmos.length; ++i) {
            if (reflectionProbeGizmos[i].targetNode === node) {
                reflectionProbeGizmos[i].locked = _generalHelper.isLocked(node);
                return;
            }
        }
    }

    function handleHiddenStateChange(node)
    {
        let i = 0;
        for (; i < lightIconGizmos.length; ++i) {
            if (lightIconGizmos[i].targetNode === node) {
                lightIconGizmos[i].hidden = _generalHelper.isHidden(node);
                return;
            }
        }
        for (i = 0; i < cameraGizmos.length; ++i) {
            if (cameraGizmos[i].targetNode === node) {
                cameraGizmos[i].hidden = _generalHelper.isHidden(node);
                return;
            }
        }
        for (i = 0; i < particleSystemIconGizmos.length; ++i) {
            if (particleSystemIconGizmos[i].targetNode === node) {
                particleSystemIconGizmos[i].hidden = _generalHelper.isHidden(node);
                return;
            }
        }
        for (i = 0; i < particleEmitterGizmos.length; ++i) {
            if (particleEmitterGizmos[i].targetNode === node) {
                particleEmitterGizmos[i].hidden = _generalHelper.isHidden(node);
                return;
            } else if (particleEmitterGizmos[i].targetNode && particleEmitterGizmos[i].targetNode.system === node) {
                particleEmitterGizmos[i].systemHidden = _generalHelper.isHidden(node);
                return;
            }
        }
        for (i = 0; i < reflectionProbeGizmos.length; ++i) {
            if (reflectionProbeGizmos[i].targetNode === node) {
                reflectionProbeGizmos[i].hidden = _generalHelper.isHidden(node);
                return;
            }
        }
    }

    SceneEnvironment {
        id: sceneEnv
        antialiasingMode: SceneEnvironment.MSAA
        antialiasingQuality: SceneEnvironment.High
    }

    Node {
        id: sceneNode

        PerspectiveCamera {
            id: overlayPerspectiveCamera
            clipFar: overlayView.editView ? overlayView.editView.perspectiveCamera.clipFar : 1000
            clipNear: overlayView.editView ? overlayView.editView.perspectiveCamera.clipNear : 1
            position: overlayView.editView ? overlayView.editView.perspectiveCamera.position : Qt.vector3d(0, 0, 0)
            rotation: overlayView.editView ? overlayView.editView.perspectiveCamera.rotation : Qt.quaternion(1, 0, 0, 0)
        }

        OrthographicCamera {
            id: overlayOrthoCamera
            clipFar: overlayView.editView ? overlayView.editView.orthoCamera.clipFar : 1000
            clipNear: overlayView.editView ? overlayView.editView.orthoCamera.clipNear : 1
            position: overlayView.editView ? overlayView.editView.orthoCamera.position : Qt.vector3d(0, 0, 0)
            rotation: overlayView.editView ? overlayView.editView.orthoCamera.rotation : Qt.quaternion(1, 0, 0, 0)
            horizontalMagnification: overlayView.editView ? overlayView.editView.orthoCamera.horizontalMagnification : 1
            verticalMagnification: overlayView.editView ? overlayView.editView.orthoCamera.verticalMagnification : 1
        }

        MouseArea3D {
            id: gizmoDragHelper
            view3D: overlayView
        }

        AutoScaleHelper {
            id: autoScale
            view3D: overlayView
            position: moveGizmo.scenePosition
        }

        AutoScaleHelper {
            id: pivotAutoScale
            view3D: overlayView
            position: pivotLine.startPos
        }

        AutoScaleHelper {
            id: lookAtAutoScale
            view3D: overlayView
            position: lookAtGizmo.scenePosition
        }

        MoveGizmo {
            id: moveGizmo
            scale: autoScale.getScale(Qt.vector3d(5, 5, 5))
            highlightOnHover: true
            targetNode: viewRoot.selectedNode
            globalOrientation: viewRoot.globalOrientation
            visible: viewRoot.selectedNode && viewRoot.transformMode === EditView3D.TransformMode.Move
            view3D: overlayView
            dragHelper: gizmoDragHelper
            property var propertyNames: ["position"]

            onPositionCommit: {
                if (targetNode === viewRoot.multiSelectionNode)
                    overlayView.commitObjectProperty(_generalHelper.multiSelectionTargets(), propertyNames);
                else
                    overlayView.commitObjectProperty([viewRoot.selectedNode], propertyNames);
            }
            onPositionMove: {
                if (targetNode === viewRoot.multiSelectionNode)
                    overlayView.changeObjectProperty(_generalHelper.multiSelectionTargets(), propertyNames);
                else
                    overlayView.changeObjectProperty([viewRoot.selectedNode], propertyNames);
            }
        }

        ScaleGizmo {
            id: scaleGizmo
            scale: autoScale.getScale(Qt.vector3d(5, 5, 5))
            highlightOnHover: true
            targetNode: viewRoot.selectedNode
            visible: viewRoot.selectedNode && viewRoot.transformMode === EditView3D.TransformMode.Scale
            view3D: overlayView
            dragHelper: gizmoDragHelper
            property var propertyNames: ["scale"]
            property var propertyNamesMulti: ["position", "scale"]

            onScaleCommit: {
                if (targetNode === viewRoot.multiSelectionNode)
                    overlayView.commitObjectProperty(_generalHelper.multiSelectionTargets(), propertyNamesMulti);
                else
                    overlayView.commitObjectProperty([viewRoot.selectedNode], propertyNames);
            }
            onScaleChange: {
                if (targetNode === viewRoot.multiSelectionNode)
                    overlayView.changeObjectProperty(_generalHelper.multiSelectionTargets(), propertyNamesMulti);
                else
                    overlayView.changeObjectProperty([viewRoot.selectedNode], propertyNames);
            }
        }

        RotateGizmo {
            id: rotateGizmo
            scale: autoScale.getScale(Qt.vector3d(7, 7, 7))
            highlightOnHover: true
            targetNode: viewRoot.selectedNode
            globalOrientation: viewRoot.globalOrientation
            visible: viewRoot.selectedNode && viewRoot.transformMode === EditView3D.TransformMode.Rotate
            view3D: overlayView
            dragHelper: gizmoDragHelper
            property var propertyNames: ["eulerRotation"]
            property var propertyNamesMulti: ["position", "eulerRotation"]

            onRotateCommit: {
                if (targetNode === viewRoot.multiSelectionNode)
                    overlayView.commitObjectProperty(_generalHelper.multiSelectionTargets(), propertyNamesMulti);
                else
                    overlayView.commitObjectProperty([viewRoot.selectedNode], propertyNames);
            }
            onRotateChange: {
                if (targetNode === viewRoot.multiSelectionNode)
                    overlayView.changeObjectProperty(_generalHelper.multiSelectionTargets(), propertyNamesMulti);
                else
                    overlayView.changeObjectProperty([viewRoot.selectedNode], propertyNames);
            }
            onCurrentAngleChanged: rotateGizmoLabel.updateLabel()
        }

        LightGizmo {
            id: lightGizmo
            targetNode: viewRoot.selectedNode !== viewRoot.multiSelectionNode ? viewRoot.selectedNode : null
            view3D: overlayView
            dragHelper: gizmoDragHelper

            onPropertyValueCommit: (propName) => {
                overlayView.commitObjectProperty([targetNode], [propName]);
            }
            onPropertyValueChange: (propName) => {
                overlayView.changeObjectProperty([targetNode], [propName]);
            }
        }

        Line3D {
            id: pivotLine
            visible: viewRoot.selectedNode && viewRoot.selectedNode !== viewRoot.multiSelectionNode
            name: "3D Edit View Pivot Line"
            color: "#ddd600"

            startPos: viewRoot.selectedNode ? viewRoot.selectedNode.scenePosition
                                            : Qt.vector3d(0, 0, 0)

            Connections {
                target: viewRoot
                function onSelectedNodeChanged()
                {
                    pivotLine.endPos = gizmoDragHelper.pivotScenePosition(viewRoot.selectedNode);
                }
            }
            Connections {
                target: viewRoot.selectedNode
                function onSceneTransformChanged()
                {
                    pivotLine.endPos = gizmoDragHelper.pivotScenePosition(viewRoot.selectedNode);
                }
            }

            Model {
                id: pivotCap
                readonly property bool _edit3dLocked: true // Make this non-pickable
                source: "#Sphere"
                scale: pivotAutoScale.getScale(Qt.vector3d(0.03, 0.03, 0.03))
                position: pivotLine.startPos
                materials: [
                    DefaultMaterial {
                        id: lineMat
                        lighting: DefaultMaterial.NoLighting
                        cullMode: Material.NoCulling
                        diffuseColor: pivotLine.color
                    }
                ]
            }
        }

        LookAtGizmo {
            id: lookAtGizmo
            color: "#ddd600"
            scale: lookAtAutoScale.getScale(Qt.vector3d(10, 10, 10))
            visible: overlayView.viewRoot.showLookAt
                     && overlayView.isActive
                     && !overlayView.viewRoot.cameraControls[viewRoot.activeSplit].showCrosshairs
        }
    }
}
