/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

import QtQuick 2.12
import QtQuick3D 1.15
import QtQuick.Controls 2.0
import QtGraphicalEffects 1.0
import MouseArea3D 1.0

Item {
    id: viewRoot
    width: 1024
    height: 768
    visible: true

    property Node activeScene: null
    property View3D editView: null
    property string sceneId

    property bool showEditLight: false
    property bool showGrid: true
    property bool usePerspective: true
    property bool globalOrientation: false
    property alias contentItem: contentItem

    enum SelectionMode { Item, Group }
    enum TransformMode { Move, Rotate, Scale }

    property int selectionMode: EditView3D.SelectionMode.Item
    property int transformMode: EditView3D.TransformMode.Move

    property Node selectedNode: null // This is non-null only in single selection case
    property var selectedNodes: [] // All selected nodes

    property var lightIconGizmos: []
    property var cameraGizmos: []
    property var selectionBoxes: []
    property rect viewPortRect: Qt.rect(0, 0, 1000, 1000)

    signal selectionChanged(var selectedNodes)
    signal commitObjectProperty(var object, var propName)
    signal changeObjectProperty(var object, var propName)
    signal notifyActiveSceneChange()

    onUsePerspectiveChanged:    _generalHelper.storeToolState(sceneId, "usePerspective", usePerspective)
    onShowEditLightChanged:     _generalHelper.storeToolState(sceneId, "showEditLight", showEditLight)
    onGlobalOrientationChanged: _generalHelper.storeToolState(sceneId, "globalOrientation", globalOrientation)
    onShowGridChanged:          _generalHelper.storeToolState(sceneId, "showGrid", showGrid);
    onSelectionModeChanged:     _generalHelper.storeToolState(sceneId, "selectionMode", selectionMode);
    onTransformModeChanged:     _generalHelper.storeToolState(sceneId, "transformMode", transformMode);

    onActiveSceneChanged: updateActiveScene()

    function createEditView()
    {
        var component = Qt.createComponent("SceneView3D.qml");
        if (component.status === Component.Ready) {
            editView = component.createObject(viewRect,
                                              {"usePerspective": usePerspective,
                                               "showSceneLight": showEditLight,
                                               "showGrid": showGrid,
                                               "importScene": activeScene,
                                               "cameraZoomFactor": cameraControl._zoomFactor,
                                               "z": 1});
            editView.usePerspective = Qt.binding(function() {return usePerspective;});
            editView.showSceneLight = Qt.binding(function() {return showEditLight;});
            editView.showGrid = Qt.binding(function() {return showGrid;});
            editView.cameraZoomFactor = Qt.binding(function() {return cameraControl._zoomFactor;});

            selectionBoxes.length = 0;
            return true;
        }
        return false;
    }

    function updateActiveScene()
    {
        if (editView) {
            // Destroy is async, so make sure we don't get any more updates for the old editView
            _generalHelper.enableItemUpdate(editView, false);
            editView.destroy();
        }

        // importScene cannot be updated after initial set, so we need to reconstruct entire View3D
        if (createEditView()) {
            if (activeScene) {
                var toolStates = _generalHelper.getToolStates(sceneId);
                if (Object.keys(toolStates).length > 0) {
                    updateToolStates(toolStates, true);
                } else {
                    // Don't inherit the edit light state from the previous scene, but rather
                    // turn the edit light on for scenes that do not have any scene
                    // lights, and turn it off for scenes that have.
                    var hasSceneLight = false;
                    for (var i = 0; i < lightIconGizmos.length; ++i) {
                        if (lightIconGizmos[i].scene === activeScene) {
                            hasSceneLight = true;
                            break;
                        }
                    }
                    showEditLight = !hasSceneLight;
                    storeCurrentToolStates();
                }
            } else {
                // When active scene is deleted, this function gets called by object deletion
                // handlers without going through setActiveScene, so make sure sceneId is cleared.
                sceneId = "";
                storeCurrentToolStates();
            }

            notifyActiveSceneChange();
        }
    }

    function setActiveScene(newScene, newSceneId)
    {
        var needExplicitUpdate = !activeScene && !newScene;

        sceneId = newSceneId;
        activeScene = newScene;

        if (needExplicitUpdate)
            updateActiveScene();
    }

    // Disables edit view update if scene doesn't match current activeScene.
    // If it matches, updates are enabled.
    function enableEditViewUpdate(scene)
    {
        if (editView)
            _generalHelper.enableItemUpdate(editView, (scene && scene === activeScene));
    }

    function handleActiveSceneIdChange(newId)
    {
        if (sceneId !== newId) {
            sceneId = newId;
            storeCurrentToolStates();
        }
    }

    function fitToView()
    {
        if (editView) {
            var targetNode = selectionBoxes.length > 0
                    ? selectionBoxes[0].model : null;
            cameraControl.focusObject(targetNode, editView.camera.eulerRotation, true, false);
        }
    }

    // If resetToDefault is true, tool states not specifically set to anything will be reset to
    // their default state.
    function updateToolStates(toolStates, resetToDefault)
    {
        if ("showEditLight" in toolStates)
            showEditLight = toolStates.showEditLight;
        else if (resetToDefault)
            showEditLight = false;

        if ("showGrid" in toolStates)
            showGrid = toolStates.showGrid;
        else if (resetToDefault)
            showGrid = true;

        if ("usePerspective" in toolStates)
            usePerspective = toolStates.usePerspective;
        else if (resetToDefault)
            usePerspective = true;

        if ("globalOrientation" in toolStates)
            globalOrientation = toolStates.globalOrientation;
        else if (resetToDefault)
            globalOrientation = false;

        if ("selectionMode" in toolStates)
            selectionMode = toolStates.selectionMode;
        else if (resetToDefault)
            selectionMode = EditView3D.SelectionMode.Item;

        if ("transformMode" in toolStates)
            transformMode = toolStates.transformMode;
        else if (resetToDefault)
            selectionMode = EditView3D.TransformMode.Move;

        if ("editCamState" in toolStates)
            cameraControl.restoreCameraState(toolStates.editCamState);
        else if (resetToDefault)
            cameraControl.restoreDefaultState();
    }

    function storeCurrentToolStates()
    {
        _generalHelper.storeToolState(sceneId, "showEditLight", showEditLight)
        _generalHelper.storeToolState(sceneId, "showGrid", showGrid)
        _generalHelper.storeToolState(sceneId, "usePerspective", usePerspective)
        _generalHelper.storeToolState(sceneId, "globalOrientation", globalOrientation)
        _generalHelper.storeToolState(sceneId, "selectionMode", selectionMode);
        _generalHelper.storeToolState(sceneId, "transformMode", transformMode);

        cameraControl.storeCameraState(0);
    }

    function ensureSelectionBoxes(count)
    {
        var needMore = count - selectionBoxes.length
        if (needMore > 0) {
            var component = Qt.createComponent("SelectionBox.qml");
            if (component.status === Component.Ready) {
                for (var i = 0; i < needMore; ++i) {
                    var geometryName = _generalHelper.generateUniqueName("SelectionBoxGeometry");
                    var boxParent = null;
                    if (editView)
                        boxParent = editView.sceneHelpers;
                    var box = component.createObject(boxParent, {"view3D": editView,
                                                     "geometryName": geometryName});
                    selectionBoxes[selectionBoxes.length] = box;
                    box.view3D = Qt.binding(function() {return editView;});
                }
            }
        }
    }

    function selectObjects(objects)
    {
        // Create selection boxes as necessary. One more box than is actually needed is created, so
        // that we always have a previously created box to use for new selection.
        // This fixes an occasional visual glitch when creating a new box.
        ensureSelectionBoxes(objects.length + 1)

        var i;
        for (i = 0; i < objects.length; ++i)
            selectionBoxes[i].targetNode = objects[i];
        for (i = objects.length; i < selectionBoxes.length; ++i)
            selectionBoxes[i].targetNode = null;

        selectedNodes = objects;
        if (objects.length === 0 || objects.length > 1)
            selectedNode = null;
        else
            selectedNode = objects[0];
    }

    function handleObjectClicked(object, multi)
    {
        var theObject = object;
        if (selectionMode === EditView3D.SelectionMode.Group) {
            while (theObject && theObject !== activeScene && theObject.parent !== activeScene)
                theObject = theObject.parent;
        }
        // Object selection logic:
        // Regular click: Clear any multiselection, single-selects the clicked object
        // Ctrl-click: No objects selected: Act as single select
        //             One or more objects selected: Multiselect
        // Null object always clears entire selection
        var newSelection = [];
        if (object !== null) {
            if (multi && selectedNodes.length > 0) {
                var deselect = false;
                for (var i = 0; i < selectedNodes.length; ++i) {
                    // Multiselecting already selected object clears that object from selection
                    if (selectedNodes[i] !== object)
                        newSelection[newSelection.length] = selectedNodes[i];
                    else
                        deselect = true;
                }
                if (!deselect)
                    newSelection[newSelection.length] = object;
            } else {
                newSelection[0] = theObject;
            }
        }
        selectObjects(newSelection);
        selectionChanged(newSelection);
    }

    function addLightGizmo(scene, obj)
    {
        // Insert into first available gizmo
        for (var i = 0; i < lightIconGizmos.length; ++i) {
            if (!lightIconGizmos[i].targetNode) {
                lightIconGizmos[i].scene = scene;
                lightIconGizmos[i].targetNode = obj;
                return;
            }
        }

        // No free gizmos available, create a new one
        var gizmoComponent = Qt.createComponent("LightIconGizmo.qml");
        if (gizmoComponent.status === Component.Ready) {
            var gizmo = gizmoComponent.createObject(overlayView,
                                                    {"view3D": overlayView, "targetNode": obj,
                                                     "selectedNodes": selectedNodes, "scene": scene,
                                                     "activeScene": activeScene});
            lightIconGizmos[lightIconGizmos.length] = gizmo;
            gizmo.clicked.connect(handleObjectClicked);
            gizmo.selectedNodes = Qt.binding(function() {return selectedNodes;});
            gizmo.activeScene = Qt.binding(function() {return activeScene;});
        }
    }

    function addCameraGizmo(scene, obj)
    {
        // Insert into first available gizmo
        for (var i = 0; i < cameraGizmos.length; ++i) {
            if (!cameraGizmos[i].targetNode) {
                cameraGizmos[i].scene = scene;
                cameraGizmos[i].targetNode = obj;
                return;
            }
        }
        // No free gizmos available, create a new one
        var gizmoComponent = Qt.createComponent("CameraGizmo.qml");
        var frustumComponent = Qt.createComponent("CameraFrustum.qml");
        if (gizmoComponent.status === Component.Ready && frustumComponent.status === Component.Ready) {
            var geometryName = _generalHelper.generateUniqueName("CameraGeometry");
            var frustum = frustumComponent.createObject(
                        overlayScene,
                        {"geometryName": geometryName, "viewPortRect": viewPortRect});
            var gizmo = gizmoComponent.createObject(
                        overlayView,
                        {"view3D": overlayView, "targetNode": obj,
                         "selectedNodes": selectedNodes, "scene": scene, "activeScene": activeScene});

            cameraGizmos[cameraGizmos.length] = gizmo;
            gizmo.clicked.connect(handleObjectClicked);
            gizmo.selectedNodes = Qt.binding(function() {return selectedNodes;});
            gizmo.activeScene = Qt.binding(function() {return activeScene;});
            frustum.viewPortRect = Qt.binding(function() {return viewPortRect;});
            gizmo.connectFrustum(frustum);
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

    function updateLightGizmoScene(scene, obj)
    {
        for (var i = 0; i < lightIconGizmos.length; ++i) {
            if (lightIconGizmos[i].targetNode === obj) {
                lightIconGizmos[i].scene = scene;
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

    Component.onCompleted: {
        createEditView();
        selectObjects([]);
        // Work-around the fact that the projection matrix for the camera is not calculated until
        // the first frame is rendered, so any initial calls to mapFrom3DScene() will fail.
        _generalHelper.requestOverlayUpdate();
    }

    onWidthChanged: _generalHelper.requestOverlayUpdate()
    onHeightChanged: _generalHelper.requestOverlayUpdate()

    Node {
        id: overlayScene

        PerspectiveCamera {
            id: overlayPerspectiveCamera
            clipFar: viewRoot.editView ? viewRoot.editView.perspectiveCamera.clipFar : 1000
            clipNear: viewRoot.editView ? viewRoot.editView.perspectiveCamera.clipNear : 1
            position: viewRoot.editView ? viewRoot.editView.perspectiveCamera.position : Qt.vector3d(0, 0, 0)
            rotation: viewRoot.editView ? viewRoot.editView.perspectiveCamera.rotation : Qt.quaternion(1, 0, 0, 0)
        }

        OrthographicCamera {
            id: overlayOrthoCamera
            clipFar: viewRoot.editView ? viewRoot.editView.orthoCamera.clipFar : 1000
            clipNear: viewRoot.editView ? viewRoot.editView.orthoCamera.clipNear : 1
            position: viewRoot.editView ? viewRoot.editView.orthoCamera.position : Qt.vector3d(0, 0, 0)
            rotation: viewRoot.editView ? viewRoot.editView.orthoCamera.rotation : Qt.quaternion(1, 0, 0, 0)
            scale: viewRoot.editView ? viewRoot.editView.orthoCamera.scale : Qt.vector3d(0, 0, 0)
        }

        MouseArea3D {
            id: gizmoDragHelper
            view3D: overlayView
        }

        MoveGizmo {
            id: moveGizmo
            scale: autoScale.getScale(Qt.vector3d(5, 5, 5))
            highlightOnHover: true
            targetNode: viewRoot.selectedNode
            globalOrientation: viewRoot.globalOrientation
            visible: viewRoot.selectedNode && transformMode === EditView3D.TransformMode.Move
            view3D: overlayView
            dragHelper: gizmoDragHelper

            onPositionCommit: viewRoot.commitObjectProperty(viewRoot.selectedNode, "position")
            onPositionMove: viewRoot.changeObjectProperty(viewRoot.selectedNode, "position")
        }

        ScaleGizmo {
            id: scaleGizmo
            scale: autoScale.getScale(Qt.vector3d(5, 5, 5))
            highlightOnHover: true
            targetNode: viewRoot.selectedNode
            visible: viewRoot.selectedNode && transformMode === EditView3D.TransformMode.Scale
            view3D: overlayView
            dragHelper: gizmoDragHelper

            onScaleCommit: viewRoot.commitObjectProperty(viewRoot.selectedNode, "scale")
            onScaleChange: viewRoot.changeObjectProperty(viewRoot.selectedNode, "scale")
        }

        RotateGizmo {
            id: rotateGizmo
            scale: autoScale.getScale(Qt.vector3d(7, 7, 7))
            highlightOnHover: true
            targetNode: viewRoot.selectedNode
            globalOrientation: viewRoot.globalOrientation
            visible: viewRoot.selectedNode && transformMode === EditView3D.TransformMode.Rotate
            view3D: overlayView
            dragHelper: gizmoDragHelper

            onRotateCommit: viewRoot.commitObjectProperty(viewRoot.selectedNode, "eulerRotation")
            onRotateChange: viewRoot.changeObjectProperty(viewRoot.selectedNode, "eulerRotation")
        }

        LightGizmo {
            id: lightGizmo
            targetNode: viewRoot.selectedNode
            view3D: overlayView
            dragHelper: gizmoDragHelper

            onPropertyValueCommit: viewRoot.commitObjectProperty(targetNode, propName)
            onPropertyValueChange: viewRoot.changeObjectProperty(targetNode, propName)
        }

        AutoScaleHelper {
            id: autoScale
            view3D: overlayView
            position: moveGizmo.scenePosition
        }

        Line3D {
            id: pivotLine
            visible: viewRoot.selectedNode
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
                source: "#Sphere"
                scale: autoScale.getScale(Qt.vector3d(0.03, 0.03, 0.03))
                position: pivotLine.startPos
                materials: [
                    DefaultMaterial {
                        id: lineMat
                        lighting: DefaultMaterial.NoLighting
                        cullMode: Material.NoCulling
                        emissiveColor: pivotLine.color
                    }
                ]
            }
        }
    }

    Item {
        id: contentItem
        anchors.fill: parent

        Rectangle {
            id: viewRect
            anchors.fill: parent

            gradient: Gradient {
                GradientStop { position: 1.0; color: "#222222" }
                GradientStop { position: 0.0; color: "#999999" }
            }

            MouseArea {
                anchors.fill: parent
                acceptedButtons: Qt.LeftButton
                hoverEnabled: false

                property MouseArea3D freeDraggerArea
                property point pressPoint
                property bool initialMoveBlock: false

                onPressed: {
                    if (viewRoot.editView) {
                        var pickResult = viewRoot.editView.pick(mouse.x, mouse.y);
                        handleObjectClicked(_generalHelper.resolvePick(pickResult.objectHit),
                                            mouse.modifiers & Qt.ControlModifier);

                        if (pickResult.objectHit) {
                            if (transformMode === EditView3D.TransformMode.Move)
                                freeDraggerArea = moveGizmo.freeDraggerArea;
                            else if (transformMode === EditView3D.TransformMode.Rotate)
                                freeDraggerArea = rotateGizmo.freeDraggerArea;
                            else if (transformMode === EditView3D.TransformMode.Scale)
                                freeDraggerArea = scaleGizmo.freeDraggerArea;
                            pressPoint.x = mouse.x;
                            pressPoint.y = mouse.y;
                            initialMoveBlock = true;
                        } else {
                            mouse.accepted = false;
                        }
                    }
                }
                onPositionChanged: {
                    if (freeDraggerArea) {
                        if (initialMoveBlock && Math.abs(pressPoint.x - mouse.x) + Math.abs(pressPoint.y - mouse.y) > 10) {
                            // Don't force press event at actual press, as that puts the gizmo
                            // in free-dragging state, which is bad UX if drag is not actually done
                            freeDraggerArea.forcePressEvent(pressPoint.x, pressPoint.y);
                            freeDraggerArea.forceMoveEvent(mouse.x, mouse.y);
                            initialMoveBlock = false;
                        } else {
                            freeDraggerArea.forceMoveEvent(mouse.x, mouse.y);
                        }
                    }
                }

                function handleRelease(mouse)
                {
                    if (freeDraggerArea) {
                        if (initialMoveBlock)
                            freeDraggerArea.forceReleaseEvent(pressPoint.x, pressPoint.y);
                        else
                            freeDraggerArea.forceReleaseEvent(mouse.x, mouse.y);
                        freeDraggerArea = null;
                    }
                }

                onReleased: handleRelease(mouse)
                onCanceled: handleRelease(mouse)
            }

            DropArea {
                anchors.fill: parent
            }

            View3D {
                id: overlayView
                anchors.fill: parent
                camera: viewRoot.usePerspective ? overlayPerspectiveCamera : overlayOrthoCamera
                importScene: overlayScene
                z: 2
            }

            Overlay2D {
                id: gizmoLabel
                targetNode: moveGizmo.visible ? moveGizmo : scaleGizmo
                targetView: overlayView
                visible: targetNode.dragging
                z: 3

                Rectangle {
                    color: "white"
                    x: -width / 2
                    y: -height - 8
                    width: gizmoLabelText.width + 4
                    height: gizmoLabelText.height + 4
                    border.width: 1
                    Text {
                        id: gizmoLabelText
                        text: {
                            var l = Qt.locale();
                            var targetProperty;
                            if (viewRoot.selectedNode) {
                                if (gizmoLabel.targetNode === moveGizmo)
                                    targetProperty = viewRoot.selectedNode.position;
                                else
                                    targetProperty = viewRoot.selectedNode.scale;
                                return qsTr("x:") + Number(targetProperty.x).toLocaleString(l, 'f', 1)
                                    + qsTr(" y:") + Number(targetProperty.y).toLocaleString(l, 'f', 1)
                                    + qsTr(" z:") + Number(targetProperty.z).toLocaleString(l, 'f', 1);
                            } else {
                                return "";
                            }
                        }
                        anchors.centerIn: parent
                    }
                }
            }

            Rectangle {
                id: rotateGizmoLabel
                color: "white"
                x: rotateGizmo.currentMousePos.x - (10 + width)
                y: rotateGizmo.currentMousePos.y - (10 + height)
                width: rotateGizmoLabelText.width + 4
                height: rotateGizmoLabelText.height + 4
                border.width: 1
                visible: rotateGizmo.dragging
                parent: rotateGizmo.view3D
                z: 3

                Text {
                    id: rotateGizmoLabelText
                    text: {
                        var l = Qt.locale();
                        if (rotateGizmo.targetNode) {
                            var degrees = rotateGizmo.currentAngle * (180 / Math.PI);
                            return Number(degrees).toLocaleString(l, 'f', 1);
                        } else {
                            return "";
                        }
                    }
                    anchors.centerIn: parent
                }
            }

            Rectangle {
                id: lightGizmoLabel
                color: "white"
                x: lightGizmo.currentMousePos.x - (10 + width)
                y: lightGizmo.currentMousePos.y - (10 + height)
                width: lightGizmoLabelText.width + 4
                height: lightGizmoLabelText.height + 4
                border.width: 1
                visible: lightGizmo.dragging
                parent: lightGizmo.view3D
                z: 3

                Text {
                    id: lightGizmoLabelText
                    text: lightGizmo.currentLabel
                    anchors.centerIn: parent
                }
            }

            EditCameraController {
                id: cameraControl
                camera: viewRoot.editView ? viewRoot.editView.camera : null
                anchors.fill: parent
                view3d: viewRoot.editView
                sceneId: viewRoot.sceneId
            }
        }

        AxisHelper {
            anchors.right: parent.right
            anchors.top: parent.top
            width: 100
            height: width
            editCameraCtrl: cameraControl
            selectedNode : viewRoot.selectedNodes.length ? selectionBoxes[0].model : null
        }

        Text {
            id: sceneLabel
            text: viewRoot.sceneId
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.margins: 4
            font.pixelSize: 14
            color: "white"
        }
    }
}
