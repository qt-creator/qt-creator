// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 6.0
import QtQuick3D 6.0
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
    property bool showSelectionBox: true
    property bool showIconGizmo: true
    property bool showCameraFrustum: false
    property bool showParticleEmitter: false
    property bool usePerspective: true
    property bool globalOrientation: false
    property alias contentItem: contentItem
    property color backgroundGradientColorStart: "#222222"
    property color backgroundGradientColorEnd: "#999999"
    property color gridColor: "#cccccc"
    property bool syncEnvBackground: false

    enum SelectionMode { Item, Group }
    enum TransformMode { Move, Rotate, Scale }

    property int selectionMode: EditView3D.SelectionMode.Item
    property int transformMode: EditView3D.TransformMode.Move

    property Node selectedNode: null // This is multiSelectionNode in multi-selection case
    property var selectedNodes: [] // All selected nodes

    property var lightIconGizmos: []
    property var cameraGizmos: []
    property var particleSystemIconGizmos: []
    property var particleEmitterGizmos: []
    property var selectionBoxes: []
    property rect viewPortRect: Qt.rect(0, 0, 1000, 1000)
    property Node activeParticleSystem: null
    property bool shuttingDown: false

    property real fps: 0

    signal selectionChanged(var selectedNodes)
    signal commitObjectProperty(var objects, var propNames)
    signal changeObjectProperty(var objects, var propNames)
    signal notifyActiveSceneChange()

    onUsePerspectiveChanged:      _generalHelper.storeToolState(sceneId, "usePerspective", usePerspective)
    onShowEditLightChanged:       _generalHelper.storeToolState(sceneId, "showEditLight", showEditLight)
    onGlobalOrientationChanged:   _generalHelper.storeToolState(sceneId, "globalOrientation", globalOrientation)
    onShowGridChanged:            _generalHelper.storeToolState(sceneId, "showGrid", showGrid);
    onSyncEnvBackgroundChanged:   _generalHelper.storeToolState(sceneId, "syncEnvBackground", syncEnvBackground);
    onShowSelectionBoxChanged:    _generalHelper.storeToolState(sceneId, "showSelectionBox", showSelectionBox);
    onShowIconGizmoChanged:       _generalHelper.storeToolState(sceneId, "showIconGizmo", showIconGizmo);
    onShowCameraFrustumChanged:   _generalHelper.storeToolState(sceneId, "showCameraFrustum", showCameraFrustum);
    onShowParticleEmitterChanged: _generalHelper.storeToolState(sceneId, "showParticleEmitter", showParticleEmitter);
    onSelectionModeChanged:       _generalHelper.storeToolState(sceneId, "selectionMode", selectionMode);
    onTransformModeChanged:       _generalHelper.storeToolState(sceneId, "transformMode", transformMode);

    onActiveSceneChanged: updateActiveScene()

    function aboutToShutDown()
    {
        shuttingDown = true;
    }

    function createEditView()
    {
        var component = Qt.createComponent("SceneView3D.qml");
        if (component.status === Component.Ready) {
            editView = component.createObject(viewRect,
                                              {"usePerspective": usePerspective,
                                               "showSceneLight": showEditLight,
                                               "showGrid": showGrid,
                                               "gridColor": gridColor,
                                               "importScene": activeScene,
                                               "cameraLookAt": cameraControl._lookAtPoint,
                                               "z": 1});
            editView.usePerspective = Qt.binding(function() {return usePerspective;});
            editView.showSceneLight = Qt.binding(function() {return showEditLight;});
            editView.showGrid = Qt.binding(function() {return showGrid;});
            editView.gridColor = Qt.binding(function() {return gridColor;});
            editView.cameraLookAt = Qt.binding(function() {return cameraControl._lookAtPoint;});

            selectionBoxes.length = 0;
            cameraControl.forceActiveFocus();
            return true;
        }
        return false;
    }

    function updateActiveScene()
    {
        if (editView) {
            editView.visible = false;
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
                // This is skipped during application shutdown, as calling QQuickText::setText()
                // during application shutdown can crash the application.
                if (!shuttingDown) {
                    sceneId = "";
                    storeCurrentToolStates();
                }
            }

            updateEnvBackground();

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
            var boxModels = [];
            if (selectedNodes.length > 1) {
                for (var i = 0; i < selectedNodes.length; ++i) {
                    if (selectionBoxes.length > i)
                        boxModels.push(selectionBoxes[i].model)
                }
            } else if (selectedNodes.length > 0 && selectionBoxes.length > 0) {
                boxModels.push(selectionBoxes[0].model);
            }
            cameraControl.focusObject(boxModels, editView.camera.eulerRotation, true, false);
        }
    }

    function alignCamerasToView(cameraNodes)
    {
        if (editView) {
            cameraControl.alignCameras(cameraNodes);
            var propertyNames = ["position", "eulerRotation"];
            viewRoot.changeObjectProperty(cameraNodes, propertyNames);
            viewRoot.commitObjectProperty(cameraNodes, propertyNames);
        }
    }

    function alignViewToCamera(cameraNodes)
    {
        if (editView)
            cameraControl.alignView(cameraNodes);
    }

    function updateBackgroundColors(colors) {
        if (colors.length === 1) {
            backgroundGradientColorStart = colors[0];
            backgroundGradientColorEnd = colors[0];
        } else {
            backgroundGradientColorStart = colors[0];
            backgroundGradientColorEnd = colors[1];
        }
    }

    function updateEnvBackground() {
        updateBackgroundColors(_generalHelper.bgColor);

        if (!editView)
            return;

        if (syncEnvBackground) {
            let bgMode = _generalHelper.sceneEnvironmentBgMode(sceneId);
            if ((!_generalHelper.sceneEnvironmentLightProbe(sceneId) && bgMode === SceneEnvironment.SkyBox)
                || (!_generalHelper.sceneEnvironmentSkyBoxCubeMap(sceneId) && bgMode === SceneEnvironment.SkyBoxCubeMap)) {
                editView.sceneEnv.backgroundMode = SceneEnvironment.Color;
            } else {
                editView.sceneEnv.backgroundMode = bgMode;
            }
            editView.sceneEnv.lightProbe = _generalHelper.sceneEnvironmentLightProbe(sceneId);
            editView.sceneEnv.skyBoxCubeMap = _generalHelper.sceneEnvironmentSkyBoxCubeMap(sceneId);
            editView.sceneEnv.clearColor = _generalHelper.sceneEnvironmentColor(sceneId);
        } else {
            editView.sceneEnv.backgroundMode = SceneEnvironment.Transparent;
            editView.sceneEnv.lightProbe = null;
            editView.sceneEnv.skyBoxCubeMap = null;
            editView.sceneEnv.clearColor = "transparent";
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

        if ("syncEnvBackground" in toolStates) {
            syncEnvBackground = toolStates.syncEnvBackground;
            updateEnvBackground();
        } else if (resetToDefault) {
            syncEnvBackground = false;
            updateEnvBackground();
        }

        if ("showSelectionBox" in toolStates)
            showSelectionBox = toolStates.showSelectionBox;
        else if (resetToDefault)
            showSelectionBox = true;

        if ("showIconGizmo" in toolStates)
            showIconGizmo = toolStates.showIconGizmo;
        else if (resetToDefault)
            showIconGizmo = true;

        if ("showCameraFrustum" in toolStates)
            showCameraFrustum = toolStates.showCameraFrustum;
        else if (resetToDefault)
            showCameraFrustum = false;

        if ("showParticleEmitter" in toolStates)
            showParticleEmitter = toolStates.showParticleEmitter;
        else if (resetToDefault)
            showParticleEmitter = false;

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
            transformMode = EditView3D.TransformMode.Move;

        if ("editCamState" in toolStates)
            cameraControl.restoreCameraState(toolStates.editCamState);
        else if (resetToDefault)
            cameraControl.restoreDefaultState();
    }

    function storeCurrentToolStates()
    {
        _generalHelper.storeToolState(sceneId, "showEditLight", showEditLight)
        _generalHelper.storeToolState(sceneId, "showGrid", showGrid)
        _generalHelper.storeToolState(sceneId, "syncEnvBackground", syncEnvBackground)
        _generalHelper.storeToolState(sceneId, "showSelectionBox", showSelectionBox)
        _generalHelper.storeToolState(sceneId, "showIconGizmo", showIconGizmo)
        _generalHelper.storeToolState(sceneId, "showCameraFrustum", showCameraFrustum)
        _generalHelper.storeToolState(sceneId, "showParticleEmitter", showParticleEmitter)
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
                    box.showBox = Qt.binding(function() {return showSelectionBox;});
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
        if (objects.length === 0) {
            selectedNode = null;
        } else if (objects.length > 1) {
            selectedNode = multiSelectionNode;
            _generalHelper.setMultiSelectionTargets(multiSelectionNode, objects);
        } else {
            selectedNode = objects[0];
        }
    }

    function handleObjectClicked(object, button, multi)
    {
        if (object instanceof View3D) {
            // View3D can be the resolved pick target in case the 3D editor is showing content
            // of a component that has View3D as root. In that case locking is resolved on C++ side
            // and we ignore multiselection.
            selectObjects([]);
            selectionChanged([object]);
            return;
        }

        var clickedObject;

        // Click on locked object is treated same as click on empty space
        if (!_generalHelper.isLocked(object))
            clickedObject = object;

        if (selectionMode === EditView3D.SelectionMode.Group) {
            while (clickedObject && clickedObject !== activeScene
                   && (activeScene instanceof Model || clickedObject.parent !== activeScene)) {
                clickedObject = clickedObject.parent;
            }
        }
        // Object selection logic:
        // Regular click: Clear any multiselection, single-selects the clicked object
        // Ctrl-click: No objects selected: Act as single select
        //             One or more objects selected: Multiselect
        // Null object always clears entire selection
        var newSelection = [];
        if (clickedObject) {
            if (button === Qt.RightButton) {
                // Right-clicking does only single selection (when clickedObject is unselected)
                // This is needed for selecting a target for the context menu
                if (!selectedNodes.includes(clickedObject))
                    newSelection[0] = clickedObject;
                else
                    newSelection = selectedNodes;
            } else if (multi && selectedNodes.length > 0) {
                var deselect = false;
                for (var i = 0; i < selectedNodes.length; ++i) {
                    // Multiselecting already selected object clears that object from selection
                    if (selectedNodes[i] !== clickedObject)
                        newSelection[newSelection.length] = selectedNodes[i];
                    else
                        deselect = true;
                }
                if (!deselect)
                    newSelection[newSelection.length] = clickedObject;
            } else {
                newSelection[0] = clickedObject;
            }
        }
        selectObjects(newSelection);
        selectionChanged(newSelection);
    }

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
                                                    {"view3D": overlayView, "targetNode": obj,
                                                     "selectedNodes": selectedNodes, "scene": scene,
                                                     "activeScene": activeScene,
                                                     "locked": _generalHelper.isLocked(obj),
                                                     "hidden": _generalHelper.isHidden(obj),
                                                     "globalShow": showIconGizmo});
            lightIconGizmos[lightIconGizmos.length] = gizmo;
            gizmo.clicked.connect(handleObjectClicked);
            gizmo.selectedNodes = Qt.binding(function() {return selectedNodes;});
            gizmo.activeScene = Qt.binding(function() {return activeScene;});
            gizmo.globalShow = Qt.binding(function() {return showIconGizmo;});
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
                        overlayScene,
                        {"geometryName": geometryName, "viewPortRect": viewPortRect});
            var gizmo = gizmoComponent.createObject(
                        overlayView,
                        {"view3D": overlayView, "targetNode": obj,
                         "selectedNodes": selectedNodes, "scene": scene, "activeScene": activeScene,
                         "locked": _generalHelper.isLocked(obj), "hidden": _generalHelper.isHidden(obj),
                         "globalShow": showIconGizmo, "globalShowFrustum": showCameraFrustum});

            cameraGizmos[cameraGizmos.length] = gizmo;
            gizmo.clicked.connect(handleObjectClicked);
            gizmo.selectedNodes = Qt.binding(function() {return selectedNodes;});
            gizmo.activeScene = Qt.binding(function() {return activeScene;});
            gizmo.globalShow = Qt.binding(function() {return showIconGizmo;});
            gizmo.globalShowFrustum = Qt.binding(function() {return showCameraFrustum;});
            frustum.viewPortRect = Qt.binding(function() {return viewPortRect;});
            gizmo.connectFrustum(frustum);
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
                                                    {"view3D": overlayView, "targetNode": obj,
                                                     "selectedNodes": selectedNodes, "scene": scene,
                                                     "activeScene": activeScene,
                                                     "locked": _generalHelper.isLocked(obj),
                                                     "hidden": _generalHelper.isHidden(obj),
                                                     "globalShow": showIconGizmo,
                                                     "activeParticleSystem": activeParticleSystem});
            particleSystemIconGizmos[particleSystemIconGizmos.length] = gizmo;
            gizmo.clicked.connect(handleObjectClicked);
            gizmo.selectedNodes = Qt.binding(function() {return selectedNodes;});
            gizmo.activeScene = Qt.binding(function() {return activeScene;});
            gizmo.globalShow = Qt.binding(function() {return showIconGizmo;});
            gizmo.activeParticleSystem = Qt.binding(function() {return activeParticleSystem;});
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
                        {"targetNode": obj, "selectedNodes": selectedNodes,
                         "activeParticleSystem": activeParticleSystem, "scene": scene,
                         "activeScene": activeScene, "hidden": _generalHelper.isHidden(obj),
                         "systemHidden": _generalHelper.isHidden(obj.system),
                         "globalShow": showParticleEmitter});

            particleEmitterGizmos[particleEmitterGizmos.length] = gizmo;
            gizmo.selectedNodes = Qt.binding(function() {return selectedNodes;});
            gizmo.activeParticleSystem = Qt.binding(function() {return activeParticleSystem;});
            gizmo.globalShow = Qt.binding(function() {return showParticleEmitter;});
            gizmo.activeScene = Qt.binding(function() {return activeScene;});
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

    function updateParticleSystemGizmoScene(scene, obj)
    {
        for (var i = 0; i < particleSystemIconGizmos.length; ++i) {
            if (particleSystemIconGizmos[i].targetNode === obj) {
                particleSystemIconGizmos[i].scene = scene;
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

    function gizmoAt(x, y)
    {
        for (var i = 0; i < lightIconGizmos.length; ++i) {
            if (lightIconGizmos[i].visible && lightIconGizmos[i].hasPoint(x, y))
                return lightIconGizmos[i].targetNode;
        }
        for (var i = 0; i < cameraGizmos.length; ++i) {
            if (cameraGizmos[i].visible && cameraGizmos[i].hasPoint(x, y))
                return cameraGizmos[i].targetNode;
        }
        for (var i = 0; i < particleSystemIconGizmos.length; ++i) {
            if (particleSystemIconGizmos[i].visible && particleSystemIconGizmos[i].hasPoint(x, y))
                return particleSystemIconGizmos[i].targetNode;
        }
        return null;
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

    Connections {
        target: _generalHelper
        function onLockedStateChanged(node)
        {
            for (var i = 0; i < cameraGizmos.length; ++i) {
                if (cameraGizmos[i].targetNode === node) {
                    cameraGizmos[i].locked = _generalHelper.isLocked(node);
                    return;
                }
            }
            for (var i = 0; i < lightIconGizmos.length; ++i) {
                if (lightIconGizmos[i].targetNode === node) {
                    lightIconGizmos[i].locked = _generalHelper.isLocked(node);
                    return;
                }
            }
            for (var i = 0; i < particleSystemIconGizmos.length; ++i) {
                if (particleSystemIconGizmos[i].targetNode === node) {
                    particleSystemIconGizmos[i].locked = _generalHelper.isLocked(node);
                    return;
                }
            }
        }

        function onHiddenStateChanged(node)
        {
            for (var i = 0; i < cameraGizmos.length; ++i) {
                if (cameraGizmos[i].targetNode === node) {
                    cameraGizmos[i].hidden = _generalHelper.isHidden(node);
                    return;
                }
            }
            for (var i = 0; i < lightIconGizmos.length; ++i) {
                if (lightIconGizmos[i].targetNode === node) {
                    lightIconGizmos[i].hidden = _generalHelper.isHidden(node);
                    return;
                }
            }
            for (var i = 0; i < particleSystemIconGizmos.length; ++i) {
                if (particleSystemIconGizmos[i].targetNode === node) {
                    particleSystemIconGizmos[i].hidden = _generalHelper.isHidden(node);
                    return;
                }
            }
            for (var i = 0; i < particleEmitterGizmos.length; ++i) {
                if (particleEmitterGizmos[i].targetNode === node) {
                    particleEmitterGizmos[i].hidden = _generalHelper.isHidden(node);
                    return;
                } else if (particleEmitterGizmos[i].targetNode && particleEmitterGizmos[i].targetNode.system === node) {
                    particleEmitterGizmos[i].systemHidden = _generalHelper.isHidden(node);
                    return;
                }
            }
        }

        function onUpdateDragTooltip()
        {
            gizmoLabel.updateLabel();
            rotateGizmoLabel.updateLabel();
        }

        function onSceneEnvDataChanged() {
            updateEnvBackground();
        }
    }

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
            horizontalMagnification: viewRoot.editView ? viewRoot.editView.orthoCamera.horizontalMagnification : 1
            verticalMagnification: viewRoot.editView ? viewRoot.editView.orthoCamera.verticalMagnification : 1
        }

        MouseArea3D {
            id: gizmoDragHelper
            view3D: overlayView
        }

        Node {
            id: multiSelectionNode
            objectName: "multiSelectionNode"
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
            property var propertyNames: ["position"]

            onPositionCommit: {
                if (targetNode == multiSelectionNode)
                    viewRoot.commitObjectProperty(_generalHelper.multiSelectionTargets(), propertyNames);
                else
                    viewRoot.commitObjectProperty([viewRoot.selectedNode], propertyNames);
            }
            onPositionMove: {
                if (targetNode == multiSelectionNode)
                    viewRoot.changeObjectProperty(_generalHelper.multiSelectionTargets(), propertyNames);
                else
                    viewRoot.changeObjectProperty([viewRoot.selectedNode], propertyNames);
            }
        }

        ScaleGizmo {
            id: scaleGizmo
            scale: autoScale.getScale(Qt.vector3d(5, 5, 5))
            highlightOnHover: true
            targetNode: viewRoot.selectedNode
            visible: viewRoot.selectedNode && transformMode === EditView3D.TransformMode.Scale
            view3D: overlayView
            dragHelper: gizmoDragHelper
            property var propertyNames: ["scale"]
            property var propertyNamesMulti: ["position", "scale"]

            onScaleCommit: {
                if (targetNode == multiSelectionNode)
                    viewRoot.commitObjectProperty(_generalHelper.multiSelectionTargets(), propertyNamesMulti);
                else
                    viewRoot.commitObjectProperty([viewRoot.selectedNode], propertyNames);
            }
            onScaleChange: {
                if (targetNode == multiSelectionNode)
                    viewRoot.changeObjectProperty(_generalHelper.multiSelectionTargets(), propertyNamesMulti);
                else
                    viewRoot.changeObjectProperty([viewRoot.selectedNode], propertyNames);
            }
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
            property var propertyNames: ["eulerRotation"]
            property var propertyNamesMulti: ["position", "eulerRotation"]

            onRotateCommit: {
                if (targetNode == multiSelectionNode)
                    viewRoot.commitObjectProperty(_generalHelper.multiSelectionTargets(), propertyNamesMulti);
                else
                    viewRoot.commitObjectProperty([viewRoot.selectedNode], propertyNames);
            }
            onRotateChange:  {
                if (targetNode == multiSelectionNode)
                    viewRoot.changeObjectProperty(_generalHelper.multiSelectionTargets(), propertyNamesMulti);
                else
                    viewRoot.changeObjectProperty([viewRoot.selectedNode], propertyNames);
            }
            onCurrentAngleChanged: rotateGizmoLabel.updateLabel()
        }

        LightGizmo {
            id: lightGizmo
            targetNode: viewRoot.selectedNode != multiSelectionNode ? viewRoot.selectedNode : null
            view3D: overlayView
            dragHelper: gizmoDragHelper

            onPropertyValueCommit: (propName) => {
                viewRoot.commitObjectProperty([targetNode], [propName]);
            }
            onPropertyValueChange: (propName) => {
                viewRoot.changeObjectProperty([targetNode], [propName]);
            }
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

        Line3D {
            id: pivotLine
            visible: viewRoot.selectedNode && viewRoot.selectedNode != multiSelectionNode
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
    }

    Item {
        id: contentItem
        anchors.fill: parent

        Rectangle {
            id: viewRect
            anchors.fill: parent

            gradient: Gradient {
                GradientStop { position: 1.0; color: backgroundGradientColorStart }
                GradientStop { position: 0.0; color: backgroundGradientColorEnd }
            }

            MouseArea {
                anchors.fill: parent
                acceptedButtons: Qt.LeftButton | Qt.RightButton
                hoverEnabled: false

                property MouseArea3D freeDraggerArea
                property point pressPoint
                property bool initialMoveBlock: false

                onPressed: (mouse) => {
                    if (viewRoot.editView) {
                        // First pick overlay to check for hits there
                        var pickResult = _generalHelper.pickViewAt(overlayView, mouse.x, mouse.y);
                        var resolvedResult = _generalHelper.resolvePick(pickResult.objectHit);
                        if (!resolvedResult) {
                            // No hits from overlay view, pick the main scene
                            pickResult = _generalHelper.pickViewAt(viewRoot.editView, mouse.x, mouse.y);
                            resolvedResult = _generalHelper.resolvePick(pickResult.objectHit);
                        }

                        handleObjectClicked(resolvedResult, mouse.button, mouse.modifiers & Qt.ControlModifier);

                        if (pickResult.objectHit && pickResult.objectHit instanceof Node) {
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
                onPositionChanged: (mouse) => {
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

                onReleased: (mouse) => {
                    handleRelease(mouse);
                }
                onCanceled: (mouse) => {
                    handleRelease(mouse);
                }
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

                environment: sceneEnv
                SceneEnvironment {
                    id: sceneEnv
                    antialiasingMode: SceneEnvironment.MSAA
                    antialiasingQuality: SceneEnvironment.High
                }
            }

            Overlay2D {
                id: gizmoLabel
                targetNode: moveGizmo.visible ? moveGizmo : scaleGizmo
                targetView: overlayView
                visible: targetNode.dragging
                z: 3

                function updateLabel()
                {
                    // This is skipped during application shutdown, as calling QQuickText::setText()
                    // during application shutdown can crash the application.
                    if (!gizmoLabel.visible || !viewRoot.selectedNode || shuttingDown)
                        return;
                    var targetProperty;
                    if (gizmoLabel.targetNode === moveGizmo)
                        gizmoLabelText.text = _generalHelper.snapPositionDragTooltip(viewRoot.selectedNode.position);
                    else
                        gizmoLabelText.text = _generalHelper.snapScaleDragTooltip(viewRoot.selectedNode.scale);
                }

                Connections {
                    target: viewRoot.selectedNode
                    function onPositionChanged() { gizmoLabel.updateLabel() }
                    function onScaleChanged() { gizmoLabel.updateLabel() }
                }

                onVisibleChanged: gizmoLabel.updateLabel()

                Rectangle {
                    color: "white"
                    x: -width / 2
                    y: -height - 8
                    width: gizmoLabelText.width + 4
                    height: gizmoLabelText.height + 4
                    border.width: 1
                    Text {
                        id: gizmoLabelText
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

                function updateLabel() {
                    // This is skipped during application shutdown, as calling QQuickText::setText()
                    // during application shutdown can crash the application.
                    if (!rotateGizmoLabel.visible || !rotateGizmo.targetNode || shuttingDown)
                        return;
                    var degrees = rotateGizmo.currentAngle * (180 / Math.PI);
                    rotateGizmoLabelText.text = _generalHelper.snapRotationDragTooltip(degrees);
                }

                onVisibleChanged: rotateGizmoLabel.updateLabel()

                Text {
                    id: rotateGizmoLabelText
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
            selectedNode: viewRoot.selectedNodes.length === 1 ? viewRoot.selectionBoxes[0].model
                                                              : viewRoot.selectedNode
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

        Text {
            id: fpsLabel
            text: viewRoot.fps
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.margins: 4
            font.pixelSize: 12
            color: "white"
            visible: viewRoot.fps > 0
        }
    }

    Keys.onPressed: (event) => {
        switch (event.key) {
        case Qt.Key_Control:
        case Qt.Key_Shift:
            gizmoLabel.updateLabel();
            rotateGizmoLabel.updateLabel();
            break;
        default:
            break;
        }
        event.accepted = false;
    }

    Keys.onReleased: (event) => {
        switch (event.key) {
        case Qt.Key_Control:
        case Qt.Key_Shift:
            gizmoLabel.updateLabel();
            rotateGizmoLabel.updateLabel();
            break;
        default:
            break;
        }
        event.accepted = false;
    }
}
