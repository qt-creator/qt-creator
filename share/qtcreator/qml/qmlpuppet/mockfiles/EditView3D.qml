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
import QtQuick.Window 2.12
import QtQuick3D 1.0
import QtQuick.Controls 2.0
import QtGraphicalEffects 1.0
import MouseArea3D 1.0

Window {
    id: viewWindow
    width: 1024
    height: 768
    minimumHeight: 200
    minimumWidth: 200
    visible: true
    title: "3D Edit View"
    // need all those flags otherwise the title bar disappears after setting WindowStaysOnTopHint flag later
    flags: Qt.Window | Qt.WindowTitleHint | Qt.WindowSystemMenuHint | Qt.WindowMinMaxButtonsHint | Qt.WindowCloseButtonHint

    property Node activeScene: null
    property View3D editView: null

    property alias showEditLight: btnEditViewLight.toggled
    property alias usePerspective: btnPerspective.toggled
    property alias globalOrientation: btnLocalGlobal.toggled

    property Node selectedNode: null // This is non-null only in single selection case
    property var selectedNodes: [] // All selected nodes

    property var lightGizmos: []
    property var cameraGizmos: []
    property var selectionBoxes: []
    property rect viewPortRect: Qt.rect(0, 0, 1000, 1000)

    signal selectionChanged(var selectedNodes)
    signal commitObjectProperty(var object, var propName)
    signal changeObjectProperty(var object, var propName)

    onUsePerspectiveChanged: _generalHelper.storeToolState("usePerspective", usePerspective)
    onShowEditLightChanged: _generalHelper.storeToolState("showEditLight", showEditLight)
    onGlobalOrientationChanged: _generalHelper.storeToolState("globalOrientation", globalOrientation)

    onActiveSceneChanged: {
        // importScene cannot be updated after initial set, so we need to reconstruct entire View3D
        var component = Qt.createComponent("SceneView3D.qml");
        if (component.status === Component.Ready) {
            var oldView = editView;

            if (editView)
                editView.visible = false;

            editView = component.createObject(viewRect,
                                              {"usePerspective": usePerspective,
                                               "showSceneLight": showEditLight,
                                               "importScene": activeScene,
                                               "z": 1});
            editView.usePerspective = Qt.binding(function() {return usePerspective;});
            editView.showSceneLight = Qt.binding(function() {return showEditLight;});

            selectionBoxes.length = 0;
            ensureSelectionBoxes(1);

            if (oldView)
                oldView.destroy();
        }
    }

    function updateToolStates(toolStates) {
        // Init the stored state so we don't unnecessarily reflect changes back to creator
        _generalHelper.initToolStates(toolStates);

        _generalHelper.restoreWindowState(viewWindow, toolStates);

        if ("showEditLight" in toolStates)
            showEditLight = toolStates.showEditLight;
        if ("usePerspective" in toolStates)
            usePerspective = toolStates.usePerspective;
        if ("globalOrientation" in toolStates)
            globalOrientation = toolStates.globalOrientation;

        var groupIndex;
        var group;
        var i;
        if ("groupSelect" in toolStates) {
            groupIndex = toolStates.groupSelect;
            group = toolbarButtons.buttonGroups["groupSelect"];
            for (i = 0; i < group.length; ++i)
                group[i].selected = (i === groupIndex);
        }
        if ("groupTransform" in toolStates) {
            groupIndex = toolStates.groupTransform;
            group = toolbarButtons.buttonGroups["groupTransform"];
            for (i = 0; i < group.length; ++i)
                group[i].selected = (i === groupIndex);
        }

        if ("editCamState" in toolStates)
            cameraControl.restoreCameraState(toolStates.editCamState);
    }

    function ensureSelectionBoxes(count) {
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

    function selectObjects(objects) {
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

    function handleObjectClicked(object, multi) {
        var theObject = object;
        if (btnSelectGroup.selected) {
            while (theObject && theObject.parent !== activeScene)
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
        for (var i = 0; i < lightGizmos.length; ++i) {
            if (!lightGizmos[i].targetNode) {
                lightGizmos[i].scene = scene;
                lightGizmos[i].targetNode = obj;
                return;
            }
        }

        // No free gizmos available, create a new one
        var component = Qt.createComponent("LightGizmo.qml");
        if (component.status === Component.Ready) {
            var gizmo = component.createObject(overlayScene,
                                               {"view3D": overlayView, "targetNode": obj,
                                                "selectedNodes": selectedNodes, "scene": scene,
                                                "activeScene": activeScene});
            lightGizmos[lightGizmos.length] = gizmo;
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
        var component = Qt.createComponent("CameraGizmo.qml");
        if (component.status === Component.Ready) {
            var geometryName = _generalHelper.generateUniqueName("CameraGeometry");
            var gizmo = component.createObject(
                        overlayScene,
                        {"view3D": overlayView, "targetNode": obj, "geometryName": geometryName,
                         "viewPortRect": viewPortRect, "selectedNodes": selectedNodes,
                         "scene": scene, "activeScene": activeScene});
            cameraGizmos[cameraGizmos.length] = gizmo;
            gizmo.clicked.connect(handleObjectClicked);
            gizmo.viewPortRect = Qt.binding(function() {return viewPortRect;});
            gizmo.selectedNodes = Qt.binding(function() {return selectedNodes;});
            gizmo.activeScene = Qt.binding(function() {return activeScene;});
        }
    }

    // Work-around the fact that the projection matrix for the camera is not calculated until
    // the first frame is rendered, so any initial calls to mapFrom3DScene() will fail.
    Component.onCompleted: {
        selectObjects([]);
        _generalHelper.requestOverlayUpdate();
    }

    onWidthChanged: {
        _generalHelper.requestOverlayUpdate();
        _generalHelper.storeWindowState(viewWindow);
    }
    onHeightChanged: {
        _generalHelper.requestOverlayUpdate();
        _generalHelper.storeWindowState(viewWindow);

    }
    onXChanged: _generalHelper.storeWindowState(viewWindow);
    onYChanged: _generalHelper.storeWindowState(viewWindow);
    onWindowStateChanged: _generalHelper.storeWindowState(viewWindow);

    Node {
        id: overlayScene

        PerspectiveCamera {
            id: overlayPerspectiveCamera
            clipFar: viewWindow.editView ? viewWindow.editView.perpectiveCamera.clipFar : 1000
            clipNear: viewWindow.editView ? viewWindow.editView.perpectiveCamera.clipNear : 1
            position: viewWindow.editView ? viewWindow.editView.perpectiveCamera.position : Qt.vector3d(0, 0, 0)
            rotation: viewWindow.editView ? viewWindow.editView.perpectiveCamera.rotation : Qt.vector3d(0, 0, 0)
        }

        OrthographicCamera {
            id: overlayOrthoCamera
            clipFar: viewWindow.editView ? viewWindow.editView.orthoCamera.clipFar : 1000
            clipNear: viewWindow.editView ? viewWindow.editView.orthoCamera.clipNear : 1
            position: viewWindow.editView ? viewWindow.editView.orthoCamera.position : Qt.vector3d(0, 0, 0)
            rotation: viewWindow.editView ? viewWindow.editView.orthoCamera.rotation : Qt.vector3d(0, 0, 0)
            scale: viewWindow.editView ? viewWindow.editView.orthoCamera.scale : Qt.vector3d(0, 0, 0)
        }

        MouseArea3D {
            id: gizmoDragHelper
            view3D: overlayView
        }

        MoveGizmo {
            id: moveGizmo
            scale: autoScale.getScale(Qt.vector3d(5, 5, 5))
            highlightOnHover: true
            targetNode: viewWindow.selectedNode
            globalOrientation: viewWindow.globalOrientation
            visible: viewWindow.selectedNode && btnMove.selected
            view3D: overlayView
            dragHelper: gizmoDragHelper

            onPositionCommit: viewWindow.commitObjectProperty(viewWindow.selectedNode, "position")
            onPositionMove: viewWindow.changeObjectProperty(viewWindow.selectedNode, "position")
        }

        ScaleGizmo {
            id: scaleGizmo
            scale: autoScale.getScale(Qt.vector3d(5, 5, 5))
            highlightOnHover: true
            targetNode: viewWindow.selectedNode
            visible: viewWindow.selectedNode && btnScale.selected
            view3D: overlayView
            dragHelper: gizmoDragHelper

            onScaleCommit: viewWindow.commitObjectProperty(viewWindow.selectedNode, "scale")
            onScaleChange: viewWindow.changeObjectProperty(viewWindow.selectedNode, "scale")
        }

        RotateGizmo {
            id: rotateGizmo
            scale: autoScale.getScale(Qt.vector3d(7, 7, 7))
            highlightOnHover: true
            targetNode: viewWindow.selectedNode
            globalOrientation: viewWindow.globalOrientation
            visible: viewWindow.selectedNode && btnRotate.selected
            view3D: overlayView
            dragHelper: gizmoDragHelper

            onRotateCommit: viewWindow.commitObjectProperty(viewWindow.selectedNode, "rotation")
            onRotateChange: viewWindow.changeObjectProperty(viewWindow.selectedNode, "rotation")
        }

        AutoScaleHelper {
            id: autoScale
            view3D: overlayView
            position: moveGizmo.scenePosition
            orientation: moveGizmo.orientation
        }

        Line3D {
            id: pivotLine
            visible: viewWindow.selectedNode
            name: "3D Edit View Pivot Line"
            color: "#ddd600"

            function flipIfNeeded(vec) {
                if (!viewWindow.selectedNode || viewWindow.selectedNode.orientation === Node.LeftHanded)
                    return vec;
                else
                    return Qt.vector3d(vec.x, vec.y, -vec.z);
            }

            startPos: viewWindow.selectedNode ? flipIfNeeded(viewWindow.selectedNode.scenePosition)
                                              : Qt.vector3d(0, 0, 0)
            Connections {
                target: viewWindow
                onSelectedNodeChanged: {
                    pivotLine.endPos = pivotLine.flipIfNeeded(gizmoDragHelper.pivotScenePosition(
                                                                  viewWindow.selectedNode));
                }
            }
            Connections {
                target: viewWindow.selectedNode
                onSceneTransformChanged: {
                    pivotLine.endPos = pivotLine.flipIfNeeded(gizmoDragHelper.pivotScenePosition(
                                                                  viewWindow.selectedNode));
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
                        cullingMode: Material.DisableCulling
                        emissiveColor: pivotLine.color
                    }
                ]
            }
        }
    }

    Rectangle {
        id: viewRect
        anchors.fill: parent
        focus: true

        gradient: Gradient {
            GradientStop { position: 1.0; color: "#222222" }
            GradientStop { position: 0.0; color: "#999999" }
        }

        MouseArea {
            anchors.fill: parent
            acceptedButtons: Qt.LeftButton
            onClicked: {
                if (viewWindow.editView) {
                    var pickResult = viewWindow.editView.pick(mouse.x, mouse.y);
                    handleObjectClicked(_generalHelper.resolvePick(pickResult.objectHit),
                                        mouse.modifiers & Qt.ControlModifier);
                    if (!pickResult.objectHit)
                        mouse.accepted = false;
                }
            }
        }

        DropArea {
            anchors.fill: parent
        }

        View3D {
            id: overlayView
            anchors.fill: parent
            camera: usePerspective ? overlayPerspectiveCamera : overlayOrthoCamera
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
                        if (viewWindow.selectedNode) {
                            if (gizmoLabel.targetNode === moveGizmo)
                                targetProperty = viewWindow.selectedNode.position;
                            else
                                targetProperty = viewWindow.selectedNode.scale;
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

        EditCameraController {
            id: cameraControl
            camera: viewWindow.editView ? viewWindow.editView.camera : null
            anchors.fill: parent
            view3d: viewWindow.editView
        }
    }

    Rectangle { // toolbar
        id: toolbar
        color: "#9F000000"
        width: 35
        height: toolbarButtons.height

        Column {
            id: toolbarButtons
            anchors.horizontalCenter: parent.horizontalCenter
            spacing: 5
            padding: 5

            // Button groups must be defined in parent object of buttons
            property var buttonGroups: {
                "groupSelect": [btnSelectGroup, btnSelectItem],
                "groupTransform": [btnMove, btnRotate, btnScale]
            }

            ToolBarButton {
                id: btnSelectItem
                selected: true
                tooltip: qsTr("Select Item")
                shortcut: "Q"
                currentShortcut: selected ? "" : shortcut
                tool: "item_selection"
                buttonGroup: "groupSelect"
            }

            ToolBarButton {
                id: btnSelectGroup
                tooltip: qsTr("Select Group")
                shortcut: "Q"
                currentShortcut: btnSelectItem.currentShortcut === shortcut ? "" : shortcut
                tool: "group_selection"
                buttonGroup: "groupSelect"
            }

            Rectangle { // separator
                width: 25
                height: 1
                color: "#f1f1f1"
                anchors.horizontalCenter: parent.horizontalCenter
            }

            ToolBarButton {
                id: btnMove
                selected: true
                tooltip: qsTr("Move current selection")
                shortcut: "W"
                currentShortcut: shortcut
                tool: "move"
                buttonGroup: "groupTransform"
            }

            ToolBarButton {
                id: btnRotate
                tooltip: qsTr("Rotate current selection")
                shortcut: "E"
                currentShortcut: shortcut
                tool: "rotate"
                buttonGroup: "groupTransform"
            }

            ToolBarButton {
                id: btnScale
                tooltip: qsTr("Scale current selection")
                shortcut: "R"
                currentShortcut: shortcut
                tool: "scale"
                buttonGroup: "groupTransform"
            }

            Rectangle { // separator
                width: 25
                height: 1
                color: "#f1f1f1"
                anchors.horizontalCenter: parent.horizontalCenter
            }

            ToolBarButton {
                id: btnFit
                tooltip: qsTr("Fit camera to current selection")
                shortcut: "F"
                currentShortcut: shortcut
                tool: "fit"
                togglable: false

                onSelectedChanged: {
                    if (viewWindow.editView && selected) {
                        var targetNode = viewWindow.selectedNodes.length > 0
                                ? selectionBoxes[0].model : null;
                        cameraControl.focusObject(targetNode, viewWindow.editView.camera.rotation, true);
                    }
                }
            }
        }
    }

    AxisHelper {
        anchors.right: parent.right
        anchors.top: parent.top
        width: 100
        height: width
        editCameraCtrl: cameraControl
        selectedNode : viewWindow.selectedNodes.length ? selectionBoxes[0].model : null
    }

    Rectangle { // top controls bar
        color: "#aa000000"
        width: 290
        height: btnPerspective.height + 10
        anchors.top: parent.top
        anchors.right: parent.right
        anchors.rightMargin: 100

        Row {
            padding: 5
            anchors.fill: parent
            ToggleButton {
                id: btnPerspective
                width: 105
                tooltip: qsTr("Toggle Perspective / Orthographic Projection")
                states: [{iconId: "ortho", text: qsTr("Orthographic")}, {iconId: "persp",  text: qsTr("Perspective")}]
            }

            ToggleButton {
                id: btnLocalGlobal
                width: 65
                tooltip: qsTr("Toggle Global / Local Orientation")
                states: [{iconId: "local",  text: qsTr("Local")}, {iconId: "global", text: qsTr("Global")}]
            }

            ToggleButton {
                id: btnEditViewLight
                width: 110
                toggleBackground: true
                tooltip: qsTr("Toggle Edit Light")
                states: [{iconId: "edit_light_off",  text: qsTr("Edit Light Off")}, {iconId: "edit_light_on", text: qsTr("Edit Light On")}]
            }
        }

    }

    Text {
        id: helpText

        property string modKey: _generalHelper.isMacOS ? qsTr("Option") : qsTr("Alt")

        color: "white"
        text: qsTr("Camera controls: ") + modKey
              + qsTr(" + mouse press and drag. Left: Rotate, Middle: Pan, Right/Wheel: Zoom.")
        anchors.bottom: parent.bottom
    }
}
